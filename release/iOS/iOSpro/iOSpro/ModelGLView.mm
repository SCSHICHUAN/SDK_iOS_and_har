#import "ModelGLView.h"

#include "core/gl_viewer.h"

#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES3/gl.h>
#import <QuartzCore/QuartzCore.h>

@implementation ModelGLView {
  EAGLContext *_context;
  GLuint _colorRbo;
  GLuint _depthRbo;
  GLuint _framebuffer;
  CADisplayLink *_displayLink;
  CoreGlViewer *_viewer;
  CGFloat _lastPinch;
  BOOL _shutDown;
}

@synthesize dragControlsLight = _dragControlsLight;

+ (Class)layerClass {
  return [CAEAGLLayer class];
}

- (instancetype)initWithFrame:(CGRect)frame {
  self = [super initWithFrame:frame];
  if (self) {
    [self commonInit];
  }
  return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
  self = [super initWithCoder:coder];
  if (self) {
    [self commonInit];
  }
  return self;
}

- (void)commonInit {
  _dragControlsLight = NO;
  _shutDown = NO;
  self.contentScaleFactor = UIScreen.mainScreen.scale;
  CAEAGLLayer *layer = (CAEAGLLayer *)self.layer;
  layer.opaque = YES;
  layer.drawableProperties = @{
    kEAGLDrawablePropertyRetainedBacking : @NO,
    kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8
  };

  _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (!_context || ![EAGLContext setCurrentContext:_context]) {
    NSLog(@"[ModelGLView] failed to create GLES3 context");
    return;
  }

  glGenFramebuffers(1, &_framebuffer);
  glGenRenderbuffers(1, &_colorRbo);
  glGenRenderbuffers(1, &_depthRbo);

  _viewer = core_gl_viewer_create();

  UIPanGestureRecognizer *pan =
      [[UIPanGestureRecognizer alloc] initWithTarget:self action:@selector(onPan:)];
  [self addGestureRecognizer:pan];
  UIPinchGestureRecognizer *pinch =
      [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(onPinch:)];
  [self addGestureRecognizer:pinch];
  UITapGestureRecognizer *tap =
      [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTap:)];
  [tap requireGestureRecognizerToFail:pan];
  [self addGestureRecognizer:tap];

  // DisplayLink 在 didMoveToWindow / resumeRendering 里创建，避免离屏仍跑
}

- (void)didMoveToWindow {
  [super didMoveToWindow];
  if (_shutDown) return;
  if (self.window) {
    [self resumeRendering];
  } else {
    // 离开窗口：必须 invalidate，否则 CADisplayLink 强引用 self → 退页后仍 60fps
    [self pauseRendering];
  }
}

- (void)layoutSubviews {
  [super layoutSubviews];
  if (!_shutDown) [self resizeStorage];
}

- (void)resizeStorage {
  if (!_context || _shutDown) return;
  [EAGLContext setCurrentContext:_context];

  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, _colorRbo);
  [_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRbo);

  GLint w = 0, h = 0;
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &w);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &h);
  if (w <= 0 || h <= 0) return;

  glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRbo);

  glBindRenderbuffer(GL_RENDERBUFFER, _colorRbo);
  if (_viewer) core_gl_viewer_resize(_viewer, (int)w, (int)h);
}

- (BOOL)loadModelAtPath:(NSString *)path {
  if (!_context || !_viewer || _shutDown) return NO;
  [EAGLContext setCurrentContext:_context];
  int rc = core_gl_viewer_load(_viewer, path.fileSystemRepresentation);
  return rc == 0;
}

- (NSInteger)animationCount {
  if (!_viewer) return 0;
  return (NSInteger)core_gl_viewer_animation_count(_viewer);
}

- (NSArray<NSString *> *)animationNames {
  NSMutableArray<NSString *> *names = [NSMutableArray array];
  if (!_viewer) return names;
  const int n = core_gl_viewer_animation_count(_viewer);
  for (int i = 0; i < n; ++i) {
    const char *raw = core_gl_viewer_animation_name(_viewer, i);
    NSString *label = raw ? [NSString stringWithUTF8String:raw]
                          : [NSString stringWithFormat:@"Anim %d", i];
    // FBX often uses "Armature|Walk" — show the last segment (same as AR).
    NSRange pipe = [label rangeOfString:@"|" options:NSBackwardsSearch];
    if (pipe.location != NSNotFound) {
      label = [label substringFromIndex:pipe.location + 1];
    }
    [names addObject:label];
  }
  return names;
}

- (BOOL)playAnimationAtIndex:(NSInteger)index {
  if (!_viewer) return NO;
  return core_gl_viewer_play_animation(_viewer, (int)index, 1) == 0;
}

- (void)dolly:(float)amount {
  if (_viewer) core_gl_viewer_dolly(_viewer, amount);
}

- (void)cameraPan:(float)amount {
  if (_viewer) core_gl_viewer_camera_pan(_viewer, amount);
}

- (void)lightDolly:(float)amount {
  if (_viewer) core_gl_viewer_light_move_z(_viewer, amount);
}

- (void)explode {
  if (_viewer) core_gl_viewer_explode(_viewer, 1.0f);
}

- (void)implode {
  if (_viewer) core_gl_viewer_implode(_viewer);
}

- (NSInteger)pickAtX:(CGFloat)x y:(CGFloat)y {
  if (!_viewer) return -1;
  // 逻辑点 → 像素（与 glViewport / Ray::fromScreen 一致）
  const CGFloat scale = self.contentScaleFactor;
  return (NSInteger)core_gl_viewer_pick(_viewer, (float)(x * scale), (float)(y * scale));
}

- (void)clearSelection {
  if (_viewer) core_gl_viewer_clear_selection(_viewer);
}

- (void)setDirLightEnabled:(BOOL)enabled {
  if (_viewer) core_gl_viewer_set_dir_light(_viewer, enabled ? 1 : 0);
}

- (void)setSpotLightEnabled:(BOOL)enabled {
  if (_viewer) core_gl_viewer_set_spot_light(_viewer, enabled ? 1 : 0);
}

- (void)setPointLightEnabled:(BOOL)enabled {
  if (_viewer) core_gl_viewer_set_point_light(_viewer, enabled ? 1 : 0);
}

- (void)setPointLightColorR:(float)r g:(float)g b:(float)b {
  if (_viewer) core_gl_viewer_set_point_light_color(_viewer, r, g, b);
}

- (void)onPan:(UIPanGestureRecognizer *)gr {
  if (!_viewer) return;
  if (gr.state == UIGestureRecognizerStateEnded ||
      gr.state == UIGestureRecognizerStateCancelled) {
    if (!_dragControlsLight) core_gl_viewer_orbit_end(_viewer);
    return;
  }
  CGPoint d = [gr translationInView:self];
  [gr setTranslation:CGPointZero inView:self];
  // Object: (dx, dy). Light: flip Y only → (dx, -dy).
  const float dx = (float)d.x;
  const float dy = (float)d.y;
  if (_dragControlsLight) {
    core_gl_viewer_light_drag(_viewer, dx, -dy);
  } else {
    core_gl_viewer_orbit(_viewer, dx, dy);
  }
}

- (void)onPinch:(UIPinchGestureRecognizer *)gr {
  if (gr.state == UIGestureRecognizerStateBegan) {
    _lastPinch = gr.scale;
    return;
  }
  CGFloat s = gr.scale / MAX(_lastPinch, 0.0001);
  _lastPinch = gr.scale;
  if (_viewer) core_gl_viewer_zoom(_viewer, (float)s);
}

- (void)onTap:(UITapGestureRecognizer *)gr {
  if (!_viewer || gr.state != UIGestureRecognizerStateEnded) return;
  // 控灯时不点选，避免误触
  if (_dragControlsLight) return;
  CGPoint p = [gr locationInView:self];
  const NSInteger hit = [self pickAtX:p.x y:p.y];
  if (hit < 0) [self clearSelection];
  if (self.onPick) self.onPick(hit);
}

- (void)onTick:(CADisplayLink *)link {
  (void)link;
  if (!_context || !_viewer || _shutDown || !self.window) return;
  [EAGLContext setCurrentContext:_context];
  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  core_gl_viewer_render(_viewer);
  glBindRenderbuffer(GL_RENDERBUFFER, _colorRbo);
  [_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)pauseRendering {
  [_displayLink invalidate];
  _displayLink = nil;
}

- (void)resumeRendering {
  if (_shutDown || !_context) return;
  if (_displayLink) {
    _displayLink.paused = NO;
    return;
  }
  _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onTick:)];
  // 优先与屏刷同步；大模型可再考虑降帧，先保证离屏必停
  [_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)shutdown {
  if (_shutDown) return;
  _shutDown = YES;
  [self pauseRendering];

  if (_context) {
    [EAGLContext setCurrentContext:_context];
    if (_viewer) {
      core_gl_viewer_destroy(_viewer);
      _viewer = nullptr;
    }
    if (_framebuffer) {
      glDeleteFramebuffers(1, &_framebuffer);
      _framebuffer = 0;
    }
    if (_colorRbo) {
      glDeleteRenderbuffers(1, &_colorRbo);
      _colorRbo = 0;
    }
    if (_depthRbo) {
      glDeleteRenderbuffers(1, &_depthRbo);
      _depthRbo = 0;
    }
    if ([EAGLContext currentContext] == _context) {
      [EAGLContext setCurrentContext:nil];
    }
  }
  _context = nil;
}

- (void)dealloc {
  [self shutdown];
}

@end
