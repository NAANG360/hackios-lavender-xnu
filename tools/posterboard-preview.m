// iOS 16 PosterBoard preview fallback for ports without a Metal command queue.
#import <CoreGraphics/CoreGraphics.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#include <math.h>

static UIImage *render_preview(id builder, SEL selector, UIView *view)
{
    CALayer *layer = view.layer;
    CGRect bounds = layer.bounds;
    CGFloat scale = layer.contentsScale;
    if (!isfinite(bounds.origin.x) || !isfinite(bounds.origin.y) ||
        !isfinite(bounds.size.width) || !isfinite(bounds.size.height) ||
        CGRectIsEmpty(bounds) || !isfinite(scale) || scale <= 0) {
        return nil;
    }

    UIGraphicsImageRendererFormat *format = [UIGraphicsImageRendererFormat new];
    format.scale = scale;
    UIGraphicsImageRenderer *renderer = [[UIGraphicsImageRenderer alloc]
        initWithSize:bounds.size format:format];
    return [renderer imageWithActions:^(UIGraphicsImageRendererContext *context) {
        CGContextRef graphics = context.CGContext;
        CGContextTranslateCTM(graphics, -bounds.origin.x, -bounds.origin.y);
        [layer renderInContext:graphics];
    }];
}

static BOOL install_preview_fallback(void)
{
    Class builder = NSClassFromString(@"PBFHomeScreenPreviewImageBuilder");
    SEL selector = NSSelectorFromString(@"_imageForView:");
    Method method = builder ? class_getInstanceMethod(builder, selector) : NULL;
    if (!method || method_getNumberOfArguments(method) != 3) {
        return NO;
    }

    char result_type[8];
    char view_type[8];
    method_getReturnType(method, result_type, sizeof(result_type));
    method_getArgumentType(method, 2, view_type, sizeof(view_type));
    if (result_type[0] != '@' || view_type[0] != '@') {
        return NO;
    }

    method_setImplementation(method, (IMP)render_preview);
    return YES;
}

static BOOL has_metal_queue(void)
{
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    return [device newCommandQueue] != nil;
}

#if defined(POSTERBOARD_PREVIEW_PROBE)
#include <dlfcn.h>
#import <objc/message.h>

int main(void)
{
    @autoreleasepool {
        if (!dlopen("/System/Library/PrivateFrameworks/PosterBoard.framework/PosterBoard",
                    RTLD_NOW)) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }
        if (has_metal_queue() || !install_preview_fallback()) {
            fprintf(stderr, "Expected a compatible preview method and no Metal queue\n");
            return 1;
        }

        Class builder = NSClassFromString(@"PBFHomeScreenPreviewImageBuilder");
        SEL selector = NSSelectorFromString(@"_imageForView:");
        id instance = class_createInstance(builder, 0);
        UIView *view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 80, 120)];
        view.backgroundColor = UIColor.systemBlueColor;
        view.layer.contentsScale = 2;
        UIImage *(*image_for_view)(id, SEL, UIView *) =
            (UIImage *(*)(id, SEL, UIView *))objc_msgSend;
        UIImage *image = image_for_view(instance, selector, view);
        if (!image.CGImage || CGImageGetWidth(image.CGImage) != 160 ||
            CGImageGetHeight(image.CGImage) != 240) {
            fprintf(stderr, "Invalid CPU preview\n");
            return 1;
        }
        NSData *png = UIImagePNGRepresentation(image);
        view.backgroundColor = UIColor.systemRedColor;
        NSData *red_png = UIImagePNGRepresentation(image_for_view(instance, selector, view));
        if (!png.length || !red_png.length || [png isEqualToData:red_png]) {
            fprintf(stderr, "Preview pixels did not reflect the view color\n");
            return 1;
        }
        printf("PosterBoard preview by CPU: 160x240, %lu PNG bytes\n",
               (unsigned long)png.length);
        return 0;
    }
}
#else
__attribute__((constructor))
static void posterboard_preview_init(void)
{
    @autoreleasepool {
        NSString *executable = NSBundle.mainBundle.executablePath.lastPathComponent;
        if ([executable isEqualToString:@"PosterBoard"] && !has_metal_queue()) {
            BOOL installed = install_preview_fallback();
            NSLog(@"PosterBoard CPU preview fallback: %@", installed ? @"installed" : @"unavailable");
        }
    }
}
#endif
