#pragma once

// Define your BugSplat database, application name, and application version by updating the lines below
//#define BUGSPLAT_DATABASE L"Fred"
#define APPLICATION_NAME L"MyDotNetFrameworkHostCrasher"
#define APPLICATION_VERSION L"9.0.0"

#ifndef BUGSPLAT_DATABASE
#error "Please define a BugSplat database"
#endif

// The managed half of the sample, built from ..\MyDotNetFrameworkHostCrasherManaged and copied next to the exe
#define MANAGED_ASSEMBLY L"MyDotNetFrameworkHostCrasherManaged"
