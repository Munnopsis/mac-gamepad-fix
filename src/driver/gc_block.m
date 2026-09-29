#import <Foundation/Foundation.h>
#import <GameController/GameController.h>
#import <objc/runtime.h>

typedef id (*AddObserverFn)(id, SEL, NSNotificationName, id, NSOperationQueue *, void (^)(NSNotification *));

static AddObserverFn originalAddObserver = NULL;

static void logLine(NSString *s)
{
    FILE *f = fopen("/tmp/cm-gcdisable.log", "a");
    if (!f) return;
    fprintf(f, "%s\n", s.UTF8String);
    fclose(f);
}

static id cmAddObserver(id self,
                        SEL cmd,
                        NSNotificationName name,
                        id object,
                        NSOperationQueue *queue,
                        void (^block)(NSNotification *))
{
    if ([name isEqualToString:GCControllerDidConnectNotification]) {
        logLine(@"BLOCKED GCControllerDidConnectNotification");
        return originalAddObserver(
            self,
            cmd,
            @"CM.Disabled.GC.Connect",
            object,
            queue,
            block
        );
    }

    if ([name isEqualToString:GCControllerDidDisconnectNotification]) {
        logLine(@"BLOCKED GCControllerDidDisconnectNotification");
        return originalAddObserver(
            self,
            cmd,
            @"CM.Disabled.GC.Disconnect",
            object,
            queue,
            block
        );
    }

    return originalAddObserver(self, cmd, name, object, queue, block);
}

static NSArray *cmControllers(id self, SEL cmd)
{
    logLine(@"GCController.controllers -> EMPTY");
    return @[];
}

__attribute__((constructor))
static void install(void)
{
    @autoreleasepool {
        Method observerMethod =
            class_getInstanceMethod(
                [NSNotificationCenter class],
                @selector(addObserverForName:object:queue:usingBlock:)
            );

        if (observerMethod) {
            originalAddObserver =
                (AddObserverFn)method_getImplementation(observerMethod);

            method_setImplementation(
                observerMethod,
                (IMP)cmAddObserver
            );
        }

        Method controllersMethod =
            class_getClassMethod(
                [GCController class],
                @selector(controllers)
            );

        if (controllersMethod) {
            method_setImplementation(
                controllersMethod,
                (IMP)cmControllers
            );
        }

        logLine(@"GCDisable installed");
    }
}
