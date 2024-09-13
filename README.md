# JRTDF - Just Rename The Darn File

Automatically confirms the Windows Explorer file extension rename prompt:

> "If you change a file name extension, the file might become unusable.
> Are you sure you want to change it?"

Inspired by [this post](https://superuser.com/questions/67449/turn-off-change-file-extension-warning-in-windows-7) on superuser.

Please be aware that, like all the other solutions on that page, this function will click any win32 `Button` titled `&Yes` on any dialog spawned by `explorer.exe` with the exact title: `Rename`.
There is a small possibility that Explorer might create such a dialog for an operation that isn't related to renaming a file's extension.
To be thorough, this program should probably check the message contents of the dialog before clicking the button. This would virtually eliminate the possibility of mistaken dialog identity.
However, I was willing to accept this small risk for personal use, since I was programming this project within a limited amount of time.

PRs are welcome.
