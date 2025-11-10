[![bugsplat-github-banner-basic-outline](https://user-images.githubusercontent.com/20464226/149019306-3186103c-5315-4dad-a499-4fd1df408475.png)](https://bugsplat.com)
<br/>

# <div align="center">BugSplat</div>

### **<div align="center">Crash and error reporting built for busy developers.</div>**

<div align="center">
    <a href="https://bsky.app/profile/bugsplatco.bsky.social"><img alt="Follow @bugsplatco on Bluesky" src="https://img.shields.io/badge/dynamic/json?url=https%3A%2F%2Fpublic.api.bsky.app%2Fxrpc%2Fapp.bsky.actor.getProfile%2F%3Factor%3Dbugsplatco.bsky.social&query=%24.followersCount&style=social&logo=bluesky&label=Follow%20%40bugsplatco.bsky.social"></a>
    <a href="https://discord.gg/bugsplat"><img alt="Join BugSplat on Discord" src="https://img.shields.io/discord/664965194799251487?label=Join%20Discord&logo=Discord&style=social"></a>
</div>

<br/>

## Introduction 👋

This repository contains sample applications demonstrating how to integrate BugSplat crash reporting into Windows C++ applications. The samples include:

- **MyConsoleCrasher** - A console application demonstrating BugSplat integration with various crash types
- **MyCrasher** - An ATL/MFC Windows application sample
- **MyWinUI3Crasher** - A WinUI 3 application sample (requires WER configuration to work properly)

Each sample follows similar integration patterns. Detailed instructions are provided below for `MyConsoleCrasher`, and the similar principles apply to the other samples. Note that `MyWinUI3Crasher` requires Windows Error Reporting (WER) to be configured via registry settings as described in the Integration section.


## Sample Applications 🧑‍🏫

The following steps demonstrate how to get up and running with the `MyConsoleCrasher` sample application.

1. Open `MyConsoleCrasher.sln` with Visual Studio 2022+
2. Define values for `BUGSPLAT_DATABASE`, `APPLICATION_NAME`, and `APPLICATION_VERSION` in `Samples\MyConsoleCrasher\MyConsoleCrasher.h`
3. Create a Client ID and Client Secret pair for your BugSplat database on the Integrations page
4. Create a file `Samples\MyConsoleCrasher\Scripts\env.ps1` and populate it with the following (being sure to substitute your `your-client-id` and `your-client-secret` values from the previous step):

```powershell
$BUGSPLAT_CLIENT_ID = "your-client-id"
$BUGSPLAT_CLIENT_SECRET = "your-client-secret"
```

5. Set the command-line arguments for the project to `/MemoryException`, or one of the other supported arguments from the sample's source code, to test various crashes. To set the command-line arguments, right-click the `MyConsoleCrasher` project and select **Properties > Debugging > Command Arguments**.

6. Rebuild the project and run it outside of the Visual Studio debugger (Ctrl+F5). This is important since the debugger interferes with the BugSplat library's exception handling. You should see a dialog such as that shown below:

![BugSplat Crash Dialog](https://docs.bugsplat.com/.gitbook/assets/bugsplat-crash-dialog.png)

7. Enter some descriptive text to help you identify the crash you are about to upload. Click the `Send Error Report` button, and voilà! The report will be sent! In the BugSplat web app, look for the crash report with the description you entered.

8. Navigate to the BugSplat [Dashboard](https://app.bugsplat.com/v2/dashboard) and click the link in the ID column to view details about your crash, including the full symbolicated stack trace and various crash metadata.

Finally, experiment with other features of the library by examining the `MyConsoleCrasher` source code and supplying different command-line arguments.

## Integration 🏗️

This section explains how to modify your Microsoft Visual C++ application to upload crashes with full debug information to the BugSplat web application. Before getting started, please download the BugSplat SDK for Windows by clicking [here](https://docs.bugsplat.com/introduction/getting-started/integrations/desktop/cplusplus).

### Getting Started

> [!WARNING]
> For WinUI 3 applications (like `MyWinUI3Crasher`), BugSplat must be registered as a WER RuntimeExceptionModule. This process is demonstrated in our `MyWinUI3Crasher` sample project. The WER registry configuration is required for WinUI 3 applications to properly capture crashes. See the [Registry Configuration](#registry-configuration) section below for details.

Add BugSplat to your application using the following steps:

1. **Link with `BugSplat.lib`** by adding an entry to `Linker > Input > Additional Dependencies`.

2. **Add redistributable files** to your application's installer:
   - `BugSplatMonitor.exe`
   - `BugSplatWer.dll`
   - `BugSplatRc.dll`

3. **Ensure your installer runs with Administrator privileges** and creates a `RuntimeExceptionHelperModules` registry key with a name containing the full path to `BugSplatWer.dll`. For more information about configuring WER see [this doc](https://learn.microsoft.com/en-us/windows/win32/wer/wer-settings).

4. **Include `BugSplat.h`** in your application's source.

5. **Create an instance of `BugSplat`** following the example in MyConsoleCrasher. The BugSplat constructor requires three parameters: `database`, `application`, and `version`. A new BugSplat database can be created on the Database page. Choose values for application name and version to match your product release. These same values are typically used when uploading symbol files for your application. To learn more about symbol uploads, please see our [docs](https://docs.bugsplat.com/education/faq/how-to-upload-symbol-files-with-symbol-upload).

```cpp
#include "BugSplat.h"

BugSplat g_BugSplat(BUGSPLAT_DATABASE, APPLICATION_NAME, APPLICATION_VERSION);
```

6. **Modify your build settings** so that symbol files are created for release builds. Set `C/C++ > General > Debug Information Format` to `Program Database /Zi`. Be sure to also set `Linker > Debugging > Generate Debug Info` to `Yes (/DEBUG)`.

7. **Configure a Post-Build step** to upload your application's symbol files. Your script should authenticate using OAuth Client Credentials. You can create credentials on the Integrations page under the OAuth tab.

```
symbol-upload-windows.exe -b your-database -a your-app -v your-version -i your-client-id -s your-client-secret -d "$(OutputDir)\"
```

To get complete symbolicated call stacks and variable names for each crash, you should upload all `**.exe**`, `**.dll**`, and `**.pdb**` files for your product every time you build a release version of your application for distribution or internal testing.

### Registry Configuration

> [!WARNING]
> The registry configuration below is required for WinUI 3 applications (like `MyWinUI3Crasher`) to properly capture crashes. It's also recommended for other application types to ensure all crash types are captured.

To enable BugSplat to register for WER callbacks, register the `BugSplatWer.dll` module as shown below. This registry entry should be a `DWORD` whose name is the path to the `BugSplatWer.dll` file installed in the same location as your executable. The value should be `0`.

Create the WER registry key at the following location:

```
Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\RuntimeExceptionHelperModules
```

Here is that value shown highlighted in the registry editor for one of our sample applications:

![WER Registry Configuration](https://docs.bugsplat.com/.gitbook/assets/wer-registry-configuration.png)

By default, WER will stop creating crash dumps after it has seen a few of the same type. To disable this, create the following registry key:

```
Computer\HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\{Application Name}
```

Add the following values under the new key you created:

| Registry Key Setting | Description | Value |
|---------------------|-------------|-------|
| DumpType | Sets the type of dump to create. | 0 |
| DumpCount | Sets the maximum number of dumps. | 0 |

Here's an example of this shown in the registry editor:

![LocalDumps Registry Configuration](https://docs.bugsplat.com/.gitbook/assets/localdumps-registry-configuration.png)

For additional information, see [Windows Error Reporting Settings](https://learn.microsoft.com/en-us/windows/win32/wer/wer-settings)

### Verification

Test your application by forcing a crash.

```cpp
*(volatile int *)0 = 0;
```

Verify that the BugSplat dialog appears, and that crashes are posted to your BugSplat account. Ensure that symbol names in the call stack are resolved correctly. If they aren't, double-check that the correct version of symbol files and all executables for your application have been uploaded to BugSplat.

If everything was configured correctly, you should see a crash report that looks like this in your BugSplat database.

![BugSplat Crash Page](https://docs.bugsplat.com/.gitbook/assets/bugsplat-crash-page.png)

### Crash Dialog

Instructions for modifying the default crash dialog can be found on the [Windows Dialog Box page](https://docs.bugsplat.com/introduction/getting-started/integrations/desktop/cplusplus).

## Additional Resources 📚

- [Address Sanitizer Reports](https://docs.bugsplat.com/introduction/getting-started/posting-a-test-crash/myconsolecrasher-c-plus-plus/address-sanitizer-reports)
- [BugSplat for Windows API Documentation](https://docs.bugsplat.com/introduction/getting-started/integrations/desktop/cplusplus/bugsplat-for-windows-api-documentation)
- [BugSplat for Windows Upgrade Guide](https://docs.bugsplat.com/introduction/getting-started/integrations/desktop/cplusplus/bugsplat-for-windows-upgrade-guide)
- [BugSplat for Windows Dependencies](https://docs.bugsplat.com/introduction/getting-started/integrations/desktop/cplusplus/bugsplat-for-windows-dependencies)

## License 🪪

See [LICENSE](LICENSE) file for details.

