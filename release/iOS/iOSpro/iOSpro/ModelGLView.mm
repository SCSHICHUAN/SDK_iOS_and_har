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

  _displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(onTick:)];
  [_displayLink addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
}

- (void)layoutSubviews {
  [super layoutSubviews];
  [self resizeStorage];
}

- (void)resizeStorage {
  if (!_context) return;
  [EAGLContext setCurrentContext:_context];

  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, _colorRbo);
  [_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(CAEAGLLayer *)self.layer];
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _colorRbo);

  GLint w = 0, h = 0;
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &w);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &h);

  glBindRenderbuffer(GL_RENDERBUFFER, _depthRbo);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthRbo);

  glBindRenderbuffer(GL_RENDERBUFFER, _colorRbo);
  if (_viewer) core_gl_viewer_resize(_viewer, (int)w, (int)h);
}

- (BOOL)loadModelAtPath:(NSString *)path {
  if (!_context || !_viewer) return NO;
  [EAGLContext setCurrentContext:_context];
  int rc = core_gl_viewer_load(_viewer, path.fileSystemRepresentation);
  return rc == 0;
}

- (void)dolly:(float)amount {
  if (_viewer) core_gl_viewer_dolly(_viewer, amount);
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

- (void)onTick:(CADisplayLink *)link {
  if (!_context || !_viewer) return;
  [EAGLContext setCurrentContext:_context];
  glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
  core_gl_viewer_render(_viewer);
  glBindRenderbuffer(GL_RENDERBUFFER, _colorRbo);
  [_context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)dealloc {
  [_displayLink invalidate];
  [EAGLContext setCurrentContext:_context];
  if (_viewer) {
    core_gl_viewer_destroy(_viewer);
    _viewer = nullptr;
  }
  if (_framebuffer) glDeleteFramebuffers(1, &_framebuffer);
  if (_colorRbo) glDeleteRenderbuffers(1, &_colorRbo);
  if (_depthRbo) glDeleteRenderbuffers(1, &_depthRbo);
  if ([EAGLContext currentContext] == _context) {
    [EAGLContext setCurrentContext:nil];
  }
}

@end
