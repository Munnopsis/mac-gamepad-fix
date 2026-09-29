#import <Foundation/Foundation.h>
#import <dispatch/dispatch.h>

extern int cm_phase1_done;
extern void cm_phase1(void);

static void cmRun(void)
{
    if (cm_phase1_done)
        return;

    cm_phase1();

    if (!cm_phase1_done) {
        dispatch_after(
            dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1 * NSEC_PER_SEC)),
            dispatch_get_main_queue(),
            ^{
                cmRun();
            }
        );
    }
}

__attribute__((constructor))
static void cmBootstrap(void)
{
    dispatch_after(
        dispatch_time(DISPATCH_TIME_NOW, (int64_t)(2 * NSEC_PER_SEC)),
        dispatch_get_main_queue(),
        ^{
            cmRun();
        }
    );
}
