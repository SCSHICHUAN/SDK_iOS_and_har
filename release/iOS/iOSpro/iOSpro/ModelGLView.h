#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/// OpenGL ES 3 view: load a model and orbit/pinch browse.
@interface ModelGLView : UIView
- (BOOL)loadModelAtPath:(NSString *)path;
/// amount > 0 moves camera forward (closer).
- (void)dolly:(float)amount;
/// When YES, pan drags the point light; otherwise rotates the box.
@property (nonatomic, assign) BOOL dragControlsLight;
@end

NS_ASSUME_NONNULL_END
