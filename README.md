# JRTDF - Just Rename The Darn File

Automatically confirms the Windows Explorer file extension rename prompt:

> "If you change a file name extension, the file might become unusable.
> Are you sure you want to change it?"

Inspired by [this post](https://superuser.com/questions/67449/turn-off-change-file-extension-warning-in-windows-7) on superuser.

The program relies on Windows' en-US localized title, button label, and message for this dialog.
You would have to customize the program code and recompile the project to use it on a Windows setup using a non-English display language.

The program has no user interface. To exit the program, kill the `JRTDF.exe` process from Task Manager.

PRs are welcome.
