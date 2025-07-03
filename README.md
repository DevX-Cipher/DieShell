# DieShell
DieShell Extension 


This repository contains a Windows Explorer context menu extension implemented as a dynamic link library (DLL) in C++. The context menu extension, named "DiE," adds a custom menu item to the Windows Explorer context menu for selected files.

## Features

- **Die Command:** The context menu extension provides a "DiE" command that can be triggered on selected files in Windows Explorer.

- **Windows 11 Compatibility:** The "Die" command is conditionally displayed based on the Windows version. It is hidden on Windows 11 or later, while on earlier versions, it is shown by default.
- 
- **Powered by Detect It Easy:** The project incorporates functionality from "Detect It Easy" by Horsicq. [Link to Detect It Easy GitHub Repository](https://github.com/horsicq/DIE-engine)


## Feature Suggestions

Feel free to suggest new features or improvements. One potential feature idea is:

If you have other feature suggestions or ideas, please share them with the community. Contributions and feedback are always welcome!

## Installation

1. Compile the source code into a DLL.
2. Place the DLL in a location accessible to Windows Explorer.
3. Register the DLL using the appropriate registration method.

## Usage

1. Open Windows Explorer.
2. Select one or more files.
3. Right-click on the selected files to access the context menu.
4. Choose the "DiE" command.

## Build Requirements

- Microsoft Visual C++ Compiler
- Windows SDK

## Dependencies

- Microsoft Windows Runtime Library (WinRT)
- Windows Imaging Component (WIC)

## License

This project is licensed under the [MIT License](LICENSE).
