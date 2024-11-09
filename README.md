# Protected Folder System

A Windows 11 application that prevents unauthorized file transfers from a designated protected folder. This system monitors and restricts copy-paste operations, ensuring files remain within the specified root directory.

## Features

- Real-time monitoring of file system events within the protected folder
- Clipboard monitoring to prevent unauthorized copy-paste operations
- System-level hook implementation for comprehensive protection
- Detailed logging of all file operations and access attempts
- Support for monitoring subdirectories within the root folder
- Prevention of file transfers to external drives and cloud storage

## Prerequisites

- Windows 11 operating system
- Visual Studio 2019 or later with C++ development tools
- Windows SDK 10.0 or later
- Administrator privileges for system hook implementation

## Installation

1. Clone the repository to your local machine.
2. Open the project in code editor of your choice.
3. Compile the source code using the following command(assuming you have gcc installed on your machine):
```cpp
g++ -o name_of_application.exe monitor.cpp -luser32 -lgdi32 -lshell32 -static-libgcc -static-libstdc++ -mwindows

```
4. Run the executable with administrator privileges

## Configuration

1. Modify the `rootPath` variable in `WinMain` to set your protected directory:

```cpp
std::wstring rootPath = L"C:\\Path\\To\\Your\\Protected\\Folder";
```

2. The application will create a log file (`file_monitor.log`) in the same directory as the executable

## Technical Implementation

The application uses several Windows APIs and features:
- `ReadDirectoryChangesW` for file system monitoring
- Windows Clipboard API for intercepting copy-paste operations
- System hooks for monitoring system-level events
- Multi-threaded design for concurrent monitoring

## Logging

The application maintains a detailed log of:
- File creation events
- File modification events
- File deletion events
- File rename operations
- Clipboard operations involving protected files
- Unauthorized access attempts

## Assumptions and Limitations

1. **Administrator Privileges**: The application requires administrator privileges to function properly.

2. **Single User Mode**: The current implementation assumes single-user operation.

3. **Windows-Specific**: The application is designed specifically for Windows 11 and may not work on earlier versions.

4. **File Operations**: The following operations are monitored and restricted:
   - Copy-paste operations
   - Direct file system modifications
   - Clipboard operations

5. **Performance Impact**: While optimized, the application may have a minor impact on system performance due to continuous monitoring.

6. **Network Drives**: The application may have limited functionality with network drives depending on permissions and connectivity.

## Security Considerations

- The application operates at the system level and should be carefully deployed in production environments
- Regular updates to Windows security patches are recommended
- The log file should be protected to prevent unauthorized access to file operation history
- The application does not encrypt files; it only prevents unauthorized transfers

## Known Limitations

1. Cannot prevent screen captures or other indirect methods of data extraction
2. May not block all programmatic methods of file access
3. Does not prevent authorized users from disabling the application
4. Network shares may require additional configuration

## Troubleshooting

1. **Application fails to start:**
   - Verify administrator privileges
   - Check Windows event logs for errors
   - Ensure all prerequisites are installed

2. **Files still being copied:**
   - Verify the root path is correctly configured
   - Check the log file for any bypass attempts
   - Ensure no conflicting security software is running

3. **High CPU Usage:**
   - Reduce the scope of monitored directories
   - Check for recursive monitoring of symbolic links
   - Verify the number of files being monitored

## Contributing

Please feel free to submit pull requests or open issues for any improvements or bug fixes.

## Support

For support and feature requests, please open an issue in the repository's issue tracker.
