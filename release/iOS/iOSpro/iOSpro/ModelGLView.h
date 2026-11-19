#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

/// OpenGL ES 3 view: load a model and orbit/pinch browse.
@interface ModelGLView : UIView
- (BOOL)loadModelAtPath:(NSString *)path;
/// amount > 0 moves camera forward (closer).
- (void)dolly:(float)amount;
/// amount > 0：相机位置向右平移。
- (void)cameraPan:(float)amount;
/// amount > 0 moves point light along +Z（控制灯时的前进/后退）.
- (void)lightDolly:(float)amount;
/// 太阳（平行光）开关。
- (void)setDirLightEnabled:(BOOL)enabled;
/// 手电（聚光）开关。
- (void)setSpotLightEnabled:(BOOL)enabled;
/// 点光源「灯」开关。
- (void)setPointLightEnabled:(BOOL)enabled;
/// 点光源色卡 RGB（0~1）。
- (void)setPointLightColorR:(float)r g:(float)g b:(float)b;
/// When YES, pan drags the point light; otherwise rotates the model.
@property (nonatomic, assign) BOOL dragControlsLight;

/// Clip count after a successful load (0 if none).
- (NSInteger)animationCount;
/// Display names for Anim dropdown (empty segments after '|' stripped like AR).
- (NSArray<NSString *> *)animationNames;
/// Play clip at index; resets playhead. Returns YES on success.
- (BOOL)playAnimationAtIndex:(NSInteger)index;

/// 停止 CADisplayLink（打破对 self 的强引用环）并暂停绘制。离开页面必须调。
- (void)pauseRendering;
/// 恢复 60fps 绘制（若尚未创建 DisplayLink 则创建）。
- (void)resumeRendering;
/// 释放 viewer / FBO / context；之后不可再 draw。VC deinit 时调用。
- (void)shutdown;
@end

NS_ASSUME_NONNULL_END
