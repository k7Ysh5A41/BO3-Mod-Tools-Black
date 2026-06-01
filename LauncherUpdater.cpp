#include <windows.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
	std::wofstream gLog;

	std::wstring ToWide(const std::string& text)
	{
		return std::wstring(text.begin(), text.end());
	}

	std::wstring NowStamp()
	{
		SYSTEMTIME st{};
		GetLocalTime(&st);
		wchar_t buffer[64] = { 0 };
		swprintf_s(buffer, L"%04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		return std::wstring(buffer);
	}

	void LogLine(const std::wstring& message)
	{
		const std::wstring line = L"[" + NowStamp() + L"] " + message;
		std::wcout << line << std::endl;
		if (gLog.is_open())
		{
			gLog << line << std::endl;
			gLog.flush();
		}
	}

	std::wstring QuoteForMessage(const fs::path& path)
	{
		return path.wstring();
	}

	struct Options
	{
		DWORD pid = 0;
		fs::path installDir;
		fs::path updateDir;
		fs::path restartPath;
	};

	bool ParseOptions(int argc, wchar_t* argv[], Options& options, std::wstring& error)
	{
		for (int i = 1; i < argc; ++i)
		{
			const std::wstring arg = argv[i];
			if (arg == L"--pid")
			{
				if (i + 1 >= argc)
				{
					error = L"Missing value for --pid";
					return false;
				}
				++i;
				options.pid = static_cast<DWORD>(_wtoi(argv[i]));
			}
			else if (arg == L"--install-dir")
			{
				if (i + 1 >= argc)
				{
					error = L"Missing value for --install-dir";
					return false;
				}
				++i;
				options.installDir = fs::path(argv[i]);
			}
			else if (arg == L"--update-dir")
			{
				if (i + 1 >= argc)
				{
					error = L"Missing value for --update-dir";
					return false;
				}
				++i;
				options.updateDir = fs::path(argv[i]);
			}
			else if (arg == L"--restart")
			{
				if (i + 1 >= argc)
				{
					error = L"Missing value for --restart";
					return false;
				}
				++i;
				options.restartPath = fs::path(argv[i]);
			}
			else
			{
				error = L"Unknown argument: " + arg;
				return false;
			}
		}

		if (options.pid == 0)
		{
			error = L"--pid is required";
			return false;
		}
		if (options.installDir.empty())
		{
			error = L"--install-dir is required";
			return false;
		}
		if (options.updateDir.empty())
		{
			error = L"--update-dir is required";
			return false;
		}
		if (options.restartPath.empty())
		{
			error = L"--restart is required";
			return false;
		}

		return true;
	}

	bool WaitForProcessExit(DWORD pid, std::wstring& error)
	{
		HANDLE processHandle = OpenProcess(SYNCHRONIZE, FALSE, pid);
		if (!processHandle)
		{
			const DWORD openError = GetLastError();
			if (openError == ERROR_INVALID_PARAMETER)
				return true;
			error = L"OpenProcess failed for PID " + std::to_wstring(pid) + L" (error " + std::to_wstring(openError) + L")";
			return false;
		}

		const DWORD waitResult = WaitForSingleObject(processHandle, INFINITE);
		CloseHandle(processHandle);
		if (waitResult != WAIT_OBJECT_0)
		{
			error = L"WaitForSingleObject failed for PID " + std::to_wstring(pid) + L" (result " + std::to_wstring(waitResult) + L")";
			return false;
		}
		return true;
	}

	fs::path ResolvePayloadRoot(const fs::path& updateDir)
	{
		fs::path base = updateDir;
		const fs::path extractedDir = updateDir / "extracted";
		if (fs::exists(extractedDir) && fs::is_directory(extractedDir))
			base = extractedDir;

		std::vector<fs::path> childDirs;
		bool hasFile = false;
		for (const fs::directory_entry& entry : fs::directory_iterator(base))
		{
			if (entry.is_directory())
				childDirs.push_back(entry.path());
			else if (entry.is_regular_file())
				hasFile = true;
		}

		if (!hasFile && childDirs.size() == 1)
			return childDirs[0];
		return base;
	}

	bool BackupAndApply(const fs::path& payloadRoot,
		const fs::path& installDir,
		const fs::path& backupDir,
		std::vector<std::pair<fs::path, fs::path>>& backupPairs,
		std::vector<fs::path>& createdFiles,
		std::wstring& error)
	{
		std::error_code ec;
		fs::create_directories(backupDir, ec);
		if (ec)
		{
			error = L"Unable to create backup folder: " + QuoteForMessage(backupDir) + L" (" + ToWide(ec.message()) + L")";
			return false;
		}

		for (const fs::directory_entry& entry : fs::recursive_directory_iterator(payloadRoot))
		{
			const fs::path sourcePath = entry.path();
			const fs::path relativePath = fs::relative(sourcePath, payloadRoot, ec);
			if (ec)
			{
				error = L"Unable to compute relative path for " + QuoteForMessage(sourcePath) + L" (" + ToWide(ec.message()) + L")";
				return false;
			}
			const fs::path destinationPath = installDir / relativePath;

			if (entry.is_directory())
			{
				fs::create_directories(destinationPath, ec);
				if (ec)
				{
					error = L"Unable to create destination folder " + QuoteForMessage(destinationPath) + L" (" + ToWide(ec.message()) + L")";
					return false;
				}
				continue;
			}

			if (!entry.is_regular_file())
				continue;

			fs::create_directories(destinationPath.parent_path(), ec);
			if (ec)
			{
				error = L"Unable to create destination parent folder " + QuoteForMessage(destinationPath.parent_path()) + L" (" + ToWide(ec.message()) + L")";
				return false;
			}

			if (fs::exists(destinationPath))
			{
				const fs::path backupPath = backupDir / relativePath;
				fs::create_directories(backupPath.parent_path(), ec);
				if (ec)
				{
					error = L"Unable to create backup parent folder " + QuoteForMessage(backupPath.parent_path()) + L" (" + ToWide(ec.message()) + L")";
					return false;
				}
				fs::copy_file(destinationPath, backupPath, fs::copy_options::overwrite_existing, ec);
				if (ec)
				{
					error = L"Unable to backup file " + QuoteForMessage(destinationPath) + L" (" + ToWide(ec.message()) + L")";
					return false;
				}
				backupPairs.push_back(std::make_pair(backupPath, destinationPath));
			}
			else
			{
				createdFiles.push_back(destinationPath);
			}

			LogLine(L"Copy " + QuoteForMessage(sourcePath) + L" -> " + QuoteForMessage(destinationPath));
			fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing, ec);
			if (ec)
			{
				error = L"Unable to copy file " + QuoteForMessage(sourcePath) + L" to " + QuoteForMessage(destinationPath) + L" (" + ToWide(ec.message()) + L")";
				return false;
			}
		}

		return true;
	}

	void Rollback(const std::vector<std::pair<fs::path, fs::path>>& backupPairs, const std::vector<fs::path>& createdFiles)
	{
		std::error_code ec;
		for (auto it = createdFiles.rbegin(); it != createdFiles.rend(); ++it)
		{
			fs::remove(*it, ec);
			ec.clear();
		}

		for (auto it = backupPairs.rbegin(); it != backupPairs.rend(); ++it)
		{
			fs::create_directories(it->second.parent_path(), ec);
			ec.clear();
			fs::copy_file(it->first, it->second, fs::copy_options::overwrite_existing, ec);
			ec.clear();
		}
	}

	bool RestartLauncher(const fs::path& restartPath, std::wstring& error)
	{
		std::wstring cmdLine = L"\"" + restartPath.wstring() + L"\"";
		std::vector<wchar_t> mutableCmd(cmdLine.begin(), cmdLine.end());
		mutableCmd.push_back(L'\0');

		STARTUPINFOW startupInfo{};
		startupInfo.cb = sizeof(startupInfo);
		PROCESS_INFORMATION processInfo{};
		const fs::path restartWorkingDir = restartPath.parent_path();

		if (!CreateProcessW(NULL,
			mutableCmd.data(),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			restartWorkingDir.empty() ? NULL : restartWorkingDir.wstring().c_str(),
			&startupInfo,
			&processInfo))
		{
			error = L"Failed to restart launcher at " + QuoteForMessage(restartPath) + L" (error " + std::to_wstring(GetLastError()) + L")";
			return false;
		}

		CloseHandle(processInfo.hThread);
		CloseHandle(processInfo.hProcess);
		return true;
	}

	std::wstring UsageText()
	{
		return L"Usage:\n  LauncherUpdater.exe --pid <launcher_pid> --install-dir <path> --update-dir <path> --restart <launcher_exe_path>";
	}
}

int wmain(int argc, wchar_t* argv[])
{
	Options options;
	std::wstring parseError;
	if (!ParseOptions(argc, argv, options, parseError))
	{
		MessageBoxW(NULL, (parseError + L"\n\n" + UsageText()).c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}

	std::error_code ec;
	fs::create_directories(options.installDir, ec);
	const fs::path logPath = options.installDir / "update.log";
	gLog.open(logPath, std::ios::out | std::ios::app);
	LogLine(L"==== updater start ====");
	LogLine(L"InstallDir=" + QuoteForMessage(options.installDir));
	LogLine(L"UpdateDir=" + QuoteForMessage(options.updateDir));
	LogLine(L"Restart=" + QuoteForMessage(options.restartPath));
	LogLine(L"PID=" + std::to_wstring(options.pid));

	std::wstring error;
	if (!WaitForProcessExit(options.pid, error))
	{
		LogLine(error);
		MessageBoxW(NULL, error.c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}
	LogLine(L"Launcher process exited");

	if (!fs::exists(options.updateDir) || !fs::is_directory(options.updateDir))
	{
		error = L"Update directory does not exist: " + QuoteForMessage(options.updateDir);
		LogLine(error);
		MessageBoxW(NULL, error.c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}

	fs::path payloadRoot;
	try
	{
		payloadRoot = ResolvePayloadRoot(options.updateDir);
	}
	catch (const std::exception& ex)
	{
		error = L"Failed to resolve payload folder: " + ToWide(ex.what());
		LogLine(error);
		MessageBoxW(NULL, error.c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}

	if (!fs::exists(payloadRoot) || !fs::is_directory(payloadRoot))
	{
		error = L"Resolved payload folder is invalid: " + QuoteForMessage(payloadRoot);
		LogLine(error);
		MessageBoxW(NULL, error.c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}
	LogLine(L"PayloadRoot=" + QuoteForMessage(payloadRoot));

	SYSTEMTIME st{};
	GetLocalTime(&st);
	wchar_t backupSuffix[32] = { 0 };
	swprintf_s(backupSuffix, L"%04u%02u%02u_%02u%02u%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	const fs::path backupDir = options.installDir / (std::wstring(L"launcher_update_backup_") + backupSuffix);
	LogLine(L"BackupDir=" + QuoteForMessage(backupDir));

	std::vector<std::pair<fs::path, fs::path>> backupPairs;
	std::vector<fs::path> createdFiles;
	if (!BackupAndApply(payloadRoot, options.installDir, backupDir, backupPairs, createdFiles, error))
	{
		LogLine(L"Update apply failed: " + error);
		LogLine(L"Rolling back files");
		Rollback(backupPairs, createdFiles);
		MessageBoxW(NULL, error.c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}

	LogLine(L"Update apply succeeded");
	fs::remove_all(options.updateDir, ec);
	if (ec)
	{
		LogLine(L"Warning: unable to remove update directory " + QuoteForMessage(options.updateDir) + L" (" + ToWide(ec.message()) + L")");
		ec.clear();
	}
	fs::remove_all(backupDir, ec);
	if (ec)
	{
		LogLine(L"Warning: unable to remove backup directory " + QuoteForMessage(backupDir) + L" (" + ToWide(ec.message()) + L")");
		ec.clear();
	}

	if (!RestartLauncher(options.restartPath, error))
	{
		LogLine(error);
		MessageBoxW(NULL, error.c_str(), L"Launcher Update", MB_ICONERROR | MB_OK);
		return 1;
	}

	LogLine(L"Launcher restarted");
	LogLine(L"==== updater complete ====");
	return 0;
}
