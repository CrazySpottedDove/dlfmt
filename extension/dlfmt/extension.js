const vscode = require('vscode');
const cp = require('child_process');
const fs = require('fs');
const fsp = fs.promises;
const path = require('path');
const os = require('os');

let _concurrencyRunning = 0;
const _CONCURRENCY_MAX = 2;
const _concurrencyQueue = [];

function _acquireConcurrency() {
    if (_concurrencyRunning < _CONCURRENCY_MAX) {
        _concurrencyRunning++;
        return Promise.resolve();
    }
    return new Promise(resolve => _concurrencyQueue.push(resolve));
}
function _releaseConcurrency() {
    _concurrencyRunning--;
    if (_concurrencyQueue.length) {
        const next = _concurrencyQueue.shift();
        _concurrencyRunning++;
        next();
    }
}

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    const output = vscode.window.createOutputChannel('dlfmt');

    const formatFileCmd = vscode.commands.registerCommand('dlfmt.formatFileAuto', async (uri) => {
        try {
            const filePath = await resolveFilePath(uri);
            if (!filePath) return;
            await ensureSavedIfActive(filePath);

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--format-file', filePath, '--param', 'auto'], path.dirname(filePath), output);
            // 为避免频繁打断用户，不在自动命令时弹窗（保留输出）
        } catch (err) {
            reportError(err, output);
        }
    });

    const formatDirCmd = vscode.commands.registerCommand('dlfmt.formatDirectoryAuto', async (uri) => {
        try {
            const dirPath = await resolveDirPath(uri);
            if (!dirPath) return;

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--format-directory', dirPath, '--param', 'auto'], dirPath, output);
        } catch (err) {
            reportError(err, output);
        }
    });

    const formatFileManualCmd = vscode.commands.registerCommand('dlfmt.formatFileManual', async (uri) => {
        try {
            const filePath = await resolveFilePath(uri);
            if (!filePath) return;
            await ensureSavedIfActive(filePath);

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--format-file', filePath, '--param', 'manual'], path.dirname(filePath), output);
            vscode.window.showInformationMessage(`dlfmt: 手动格式化文件 ${path.basename(filePath)}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    const formatDirManualCmd = vscode.commands.registerCommand('dlfmt.formatDirectoryManual', async (uri) => {
        try {
            const dirPath = await resolveDirPath(uri);
            if (!dirPath) return;

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--format-directory', dirPath, '--param', 'manual'], dirPath, output);
            vscode.window.showInformationMessage(`dlfmt: 手动格式化文件夹 ${dirPath}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    const compressFileCmd = vscode.commands.registerCommand('dlfmt.compressFile', async (uri) => {
        try {
            const filePath = await resolveFilePath(uri);
            if (!filePath) return;
            await ensureSavedIfActive(filePath);

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--compress-file', filePath], path.dirname(filePath), output);
            vscode.window.showInformationMessage(`dlfmt: 已压缩文件 ${path.basename(filePath)}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    const compressDirCmd = vscode.commands.registerCommand('dlfmt.compressDirectory', async (uri) => {
        try {
            const dirPath = await resolveDirPath(uri);
            if (!dirPath) return;

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--compress-directory', dirPath], dirPath, output);
            vscode.window.showInformationMessage(`dlfmt: 已压缩文件夹 ${dirPath}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    const runJsonTaskCmd = vscode.commands.registerCommand('dlfmt.runJsonTask', async (uri) => {
        try {
            if (!uri || !uri.fsPath.endsWith('.json')) {
                vscode.window.showErrorMessage('请选择一个 JSON 文件');
                return;
            }
            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--json-task', uri.fsPath], path.dirname(uri.fsPath), output);
            vscode.window.showInformationMessage(`dlfmt: 已执行 JSON 任务 ${path.basename(uri.fsPath)}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    context.subscriptions.push(output, formatFileCmd, formatDirCmd, formatFileManualCmd, formatDirManualCmd, compressFileCmd, compressDirCmd, runJsonTaskCmd);

    async function formatDocumentEdits(document, mode, context) {
        const tmpFile = path.join(
            os.tmpdir(),
            `dlfmt_${Date.now()}_${Math.random().toString(16).slice(2)}.lua`
        );

        await fsp.writeFile(tmpFile, document.getText(), 'utf8');

        try {
            const exe = await resolveDlfmtExecutable(context, output);

            await runDlfmt(
                exe,
                ['--format-file', tmpFile, '--param', mode],
                path.dirname(tmpFile),
                output
            );

            const formatted = await fsp.readFile(tmpFile, 'utf8');
            const original = document.getText();

            if (formatted === original) {
                return [];
            }

            const fullRange = new vscode.Range(
                document.positionAt(0),
                document.positionAt(original.length)
            );

            return [
                vscode.TextEdit.replace(fullRange, formatted)
            ];
        } finally {
            try {
                await fsp.unlink(tmpFile);
            } catch (e) {
                // 忽略删除错误
            }
        }
    }

    context.subscriptions.push(
        vscode.languages.registerDocumentFormattingEditProvider(
            { language: 'lua', scheme: 'file' },
            {
                async provideDocumentFormattingEdits(document) {
                    const cfg = vscode.workspace.getConfiguration('dlfmt');
                    const mode = cfg.get('format.mode', 'auto');

                    return formatDocumentEdits(
                        document,
                        mode,
                        context
                    );
                }
            }
        )
    );
}

/**
 * 解析 dlfmt 可执行路径：优先使用设置 dlfmt.path，否则使用扩展内置二进制
 * @param {vscode.ExtensionContext} context
 * @param {vscode.OutputChannel} output
 */
async function resolveDlfmtExecutable(context, output) {
    const cfg = vscode.workspace.getConfiguration('dlfmt');
    const configured = (cfg.get('path') || '').trim();
    if (configured) {
        try {
            await fsp.access(configured);
        } catch {
            throw new Error(`设置的 dlfmt.path 不存在：${configured}`);
        }
        return configured;
    }
    const bundled = getBundledDlfmtPath(context);
    try {
        await fsp.access(bundled);
    } catch {
        throw new Error(`未找到内置 dlfmt 可执行文件：${bundled}`);
    }
    // 确保 *nix 可执行权限（尽量一次设置）
    if (process.platform !== 'win32') {
        try {
            await fsp.chmod(bundled, 0o755);
        } catch (e) {
            output.appendLine(`[警告] 设置可执行权限失败：${e.message}`);
        }
    }
    return bundled;
}

/**
 * 返回扩展内置 dlfmt 的平台路径
 */
function getBundledDlfmtPath(context) {
    const base = context.extensionPath;
    switch (process.platform) {
        case 'win32':
            return path.join(base, 'bin', 'dlfmt-windows.exe');
        case 'linux':
            return path.join(base, 'bin', 'dlfmt-linux');
        case 'darwin':
            return path.join(base, 'bin', 'dlfmt-macos');
        default:
            return path.join(base, 'bin', process.platform, process.arch, 'dlfmt');
    }
}

/**
 * 运行 dlfmt（收集输出并限制并发）
 */
async function runDlfmt(exePath, args, cwd, output) {
    await _acquireConcurrency();
    try {
        output.appendLine(`> "${exePath}" ${args.map(a => (/\s/.test(a) ? `"${a}"` : a)).join(' ')}`);
        return await new Promise((resolve, reject) => {
            const child = cp.spawn(exePath, args, {
                cwd,
                shell: false,
                windowsHide: true,
            });

            let stdout = '';
            let stderr = '';

            child.stdout.on('data', (d) => { stdout += d.toString(); });
            child.stderr.on('data', (d) => { stderr += d.toString(); });

            child.on('error', (err) => {
                const e = new Error(`${err.message}${stderr ? '\n' + stderr : ''}`);
                reject(e);
            });
            child.on('close', (code) => {
                if (stdout) output.append(stdout);
                if (stderr) output.append(stderr);
                if (code === 0) resolve();
                else reject(new Error(`dlfmt 退出代码 ${code}${stderr ? '\n' + stderr : ''}`));
            });
        });
    } finally {
        _releaseConcurrency();
    }
}

async function resolveFilePath(uri) {
    if (uri && uri.fsPath) {
        try {
            const stat = await fsp.lstat(uri.fsPath);
            if (stat.isFile()) return uri.fsPath;
        } catch { }
    }
    const editor = vscode.window.activeTextEditor;
    if (editor?.document?.uri.scheme === 'file') {
        return editor.document.fileName;
    }
    const picked = await vscode.window.showOpenDialog({
        canSelectFiles: true,
        canSelectFolders: false,
        canSelectMany: false,
        openLabel: '选择要格式化的文件',
    });
    return picked && picked.length ? picked[0].fsPath : undefined;
}

async function ensureSavedIfActive(filePath) {
    const editor = vscode.window.activeTextEditor;
    if (editor && editor.document && editor.document.fileName === filePath && editor.document.isDirty) {
        await editor.document.save();
    }
}

async function resolveDirPath(uri) {
    if (uri && uri.fsPath) {
        try {
            const stat = await fsp.lstat(uri.fsPath);
            if (stat.isDirectory()) return uri.fsPath;
        } catch { }
    }
    const folders = vscode.workspace.workspaceFolders || [];
    if (folders.length === 1) return folders[0].uri.fsPath;

    const picked = await vscode.window.showOpenDialog({
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: false,
        openLabel: '选择要格式化的文件夹',
    });
    return picked && picked.length ? picked[0].fsPath : undefined;
}

function reportError(err, output) {
    const msg = err && err.message ? err.message : String(err);
    output.appendLine(`[错误] ${msg}`);
    vscode.window.showErrorMessage(`dlfmt 失败：${msg}`);
}

// This method is called when your extension is deactivated
function deactivate() { }

module.exports = {
    activate,
    deactivate
}