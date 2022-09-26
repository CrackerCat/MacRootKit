#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/sysctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <sys/un.h>

#include <mach/mach.h> 
#include <mach/exc.h>

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>

#include <IOKit/IOKitLib.h>

#include <AppKit/AppKit.h>
#include <Foundation/Foundation.h>

@class NSString, NSArray, NSSet, NSURL;

@interface MIDaemonConfiguration
{
	BOOL _codeSigningEnforcementIsDisabled;
	BOOL _localSigningIsUnrestricted;
	BOOL _skipDeviceFamilyCheck;
	BOOL _skipThinningCheck;
	BOOL _allowPatchWithoutSinf;
	unsigned _installQOSOverride;
	long long _nSimultaneousInstallations;
	NSString *_iOSSupportVersion;
}

@property (nonatomic, readonly) unsigned installQOSOverride;
@property (nonatomic, readonly) BOOL skipDeviceFamilyCheck;
@property (nonatomic, readonly) BOOL skipThinningCheck;
@property (nonatomic, readonly) BOOL allowPatchWithoutSinf;
@property (nonatomic, readonly) BOOL codeSigningEnforcementIsDisabled;
@property (nonatomic, readonly) BOOL localSigningIsUnrestricted;
@property (nonatomic, readonly) long long nSimultaneousInstallations;
@property (nonatomic, readonly) NSString *iOSSupportVersion;
@property (nonatomic, copy, readonly) NSArray *diskImageApplicationsDirectories;
@property (nonatomic, readonly) NSSet *iOSSupportFrameworkRootDirectories;
@property (nonatomic, readonly) NSURL *extensionCachePlist;

+ (id)sharedInstance;
- (id)init;
- (BOOL)codeSigningEnforcementIsDisabled;
- (BOOL)localSigningIsUnrestricted;
- (id)diskImageAppBundleIDToInfoMap;
- (NSArray *)diskImageApplicationsDirectories;
- (id)iOSSupportRootDirectory;
- (NSSet *)iOSSupportFrameworkRootDirectories;
- (NSURL *)extensionCachePlist;
- (unsigned)installQOSOverride;
- (BOOL)skipDeviceFamilyCheck;
- (BOOL)skipThinningCheck;
- (BOOL)allowPatchWithoutSinf;
- (long long)nSimultaneousInstallations;
- (NSString *)iOSSupportVersion;

@end

bool swizzled_performVerificationWithError(id self, SEL selector, id error)
{
	return YES;
}

bool swizzled_skipDeviceFamilyCheck()
{
	return YES;
}

void swizzleImplementations()
{
	Class cls = objc_getClass("MIInstallableBundle");

	SEL originalSelector = @selector(performVerificationWithError:);
	SEL swizzledSelector = @selector(swizzled_performVerificationWithError:);

	BOOL didAddMethod = class_addMethod(cls,
										swizzledSelector,
										(IMP)  swizzled_performVerificationWithError,
										"@:@");

	if(didAddMethod)
	{
		Method originalMethod = class_getInstanceMethod(cls ,originalSelector);
		Method swizzledMethod = class_getInstanceMethod(cls, swizzledSelector);

		method_exchangeImplementations(originalMethod, swizzledMethod);
	}

	cls = objc_getClass("MIDaemonConfiguration");

	originalSelector = @selector(skipDeviceFamilyCheck);
	swizzledSelector = @selector(swizzled_skipDeviceFamilyCheck);

	didAddMethod = class_addMethod(cls,
									swizzledSelector,
									(IMP)  swizzled_skipDeviceFamilyCheck,
									"@:@");

	if(didAddMethod)
	{
		Method originalMethod = class_getInstanceMethod(cls ,originalSelector);
		Method swizzledMethod = class_getInstanceMethod(cls, swizzledSelector);

		method_exchangeImplementations(originalMethod, swizzledMethod);
	}
}

extern "C"
{
	__attribute__((constructor))
	static void initializer()
	{
		printf("[%s] initializer()\n", __FILE__);

		
	}

	__attribute__ ((destructor))
	static void finalizer()
	{
		printf("[%s] finalizer()\n", __FILE__);
	}
}