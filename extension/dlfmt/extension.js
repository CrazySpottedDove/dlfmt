const vscode = require('vscode');
const cp = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');
// ...existing code...

/**
 * @param {vscode.ExtensionContext} context
 */
function activate(context) {
    // 输出通道
    const output = vscode.window.createOutputChannel('dlfmt');

    const formatFileCmd = vscode.commands.registerCommand('dlfmt.formatFile', async (uri) => {
        try {
            const filePath = await resolveFilePath(uri);
            if (!filePath) return;
            await ensureSavedIfActive(filePath);

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--format-file', filePath], path.dirname(filePath), output);
            vscode.window.showInformationMessage(`dlfmt: 已格式化文件 ${path.basename(filePath)}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    const formatDirCmd = vscode.commands.registerCommand('dlfmt.formatDirectory', async (uri) => {
        try {
            const dirPath = await resolveDirPath(uri);
            if (!dirPath) return;

            const exe = await resolveDlfmtExecutable(context, output);
            await runDlfmt(exe, ['--format-directory', dirPath], dirPath, output);
            vscode.window.showInformationMessage(`dlfmt: 已格式化文件夹 ${dirPath}`);
        } catch (err) {
            reportError(err, output);
        }
    });

    // 新增：压缩文件命令
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

    // 新增：压缩目录命令
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

    context.subscriptions.push(output, formatFileCmd, formatDirCmd, compressFileCmd, compressDirCmd);

    // 格式化器，复用主输出通道
    const formatter = vscode.languages.registerDocumentFormattingEditProvider('lua', {
        async provideDocumentFormattingEdits(document) {
            const tmpFile = path.join(os.tmpdir(), `dlfmt_${Date.now()}_${Math.random().toString(16).slice(2)}.lua`);
            fs.writeFileSync(tmpFile, document.getText(), 'utf8');
            const exe = await resolveDlfmtExecutable(context, output); // 传递output
            try {
                await runDlfmt(exe, ['--format-file', tmpFile], path.dirname(tmpFile), output); // 传递output
                const formatted = fs.readFileSync(tmpFile, 'utf8');
                fs.unlinkSync(tmpFile);
                const fullRange = new vscode.Range(
                    document.positionAt(0),
                    document.positionAt(document.getText().length)
                );
                return [vscode.TextEdit.replace(fullRange, formatted)];
            } catch (err) {
                if (fs.existsSync(tmpFile)) fs.unlinkSync(tmpFile);
                output.show(true); // 只在出错时弹出
                throw err;
            }
        }
    });

    context.subscriptions.push(formatter);
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
        if (!fs.existsSync(configured)) {
            throw new Error(`设置的 dlfmt.path 不存在：${configured}`);
        }
        return configured;
    }
    const bundled = getBundledDlfmtPath(context);
    if (!fs.existsSync(bundled)) {
        throw new Error(`未找到内置 dlfmt 可执行文件：${bundled}`);
    }
    // 确保 *nix 可执行权限
    if (process.platform !== 'win32') {
        try {
            fs.chmodSync(bundled, 0o755);
        } catch (e) {
            output.appendLine(`[警告] 设置可执行权限失败：${e.message}`);
        }
    }
    return bundled;
}

/**
 * 返回扩展内置 dlfmt 的平台路径
 * 期望目录结构：
 * - bin/win32/dlfmt.exe
 * - bin/linux/dlfmt
 * - bin/darwin/dlfmt
 */
function getBundledDlfmtPath(context) {
    const base = context.extensionPath;
    switch (process.platform) {
        case 'win32':
            return path.join(base, 'bin', 'win32', 'dlfmt.exe');
        case 'linux':
            return path.join(base, 'bin', 'linux', 'dlfmt');
        case 'darwin':
            return path.join(base, 'bin', 'darwin', 'dlfmt');
        default:
            // 默认尝试通用 dlfmt
            return path.join(base, 'bin', process.platform, process.arch, 'dlfmt');
    }
}

/**
 * 运行 dlfmt
 */
async function runDlfmt(exePath, args, cwd, output) {
    output.appendLine(`> "${exePath}" ${args.map(a => (/\s/.test(a) ? `"${a}"` : a)).join(' ')}`);
    return new Promise((resolve, reject) => {
        const child = cp.spawn(exePath, args, {
            cwd,
            shell: false,
            windowsHide: true,
        });

        child.stdout.on('data', (d) => output.append(d.toString()));
        child.stderr.on('data', (d) => output.append(d.toString()));

        child.on('error', (err) => {
            reject(err); // 不主动弹出
        });
        child.on('close', (code) => {
            if (code === 0) resolve();
            else reject(new Error(`dlfmt 退出代码 ${code}`));
        });
    });
}

async function resolveFilePath(uri) {
    if (uri && uri.fsPath) {
        try {
            const stat = fs.lstatSync(uri.fsPath);
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
            const stat = fs.lstatSync(uri.fsPath);
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
// ...existing code...