import { glSetModelPath, glPresentAll, glOrbitAll, glDollyAll, glLightDragAll } from '@sdk/core';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { common } from '@kit.AbilityKit';
import { fileIo as fs } from '@kit.CoreFileKit';
import { SymbolGlyphModifier } from '@kit.ArkUI';
import { window } from '@kit.ArkUI';

const DOMAIN: number = 0x0000;
const IOS_CHIP_BG: string = '#73000000';
/** Matches glClearColor(0.12, 0.13, 0.16) — status/nav/title same as GL, no black strips. */
const GL_BG: string = '#1E2129';

@Component
export struct ModelGLPage {
  @State private dragControlsLight: boolean = false;
  @State private hud: string = '';
  private timerId: number = -1;
  private holdTimerId: number = -1;
  private xComponentController: XComponentController = new XComponentController();
  private dragging: boolean = false;
  private lastX: number = 0;
  private lastY: number = 0;

  aboutToAppear(): void {
    // Status transparent (title is STACK translucent like iOS); bottom nav matches GL.
    this.setSystemBars('#00000000', '#FFFFFFFF', GL_BG);
    try {
      const ctx: common.UIAbilityContext = getContext(this) as common.UIAbilityContext;
      const names: string[] = ['cube.obj', 'cube.mtl', 'box.png'];
      for (let i = 0; i < names.length; ++i) {
        const name: string = names[i];
        const raw: Uint8Array = ctx.resourceManager.getRawFileContentSync(`models/${name}`);
        const outPath: string = `${ctx.filesDir}/${name}`;
        const ab: ArrayBuffer = raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength);
        const file = fs.openSync(outPath, fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY);
        fs.writeSync(file.fd, ab);
        fs.closeSync(file);
      }
      glSetModelPath(`${ctx.filesDir}/cube.obj`);
      this.hud = 'loaded cube.obj + box.png — drag orbit, pinch zoom';
    } catch (err) {
      const msg: string = err instanceof Error ? err.message : JSON.stringify(err);
      this.hud = `load failed: ${msg}`;
      hilog.error(DOMAIN, 'CoreGl', 'model: %{public}s', msg);
    }
    this.timerId = setInterval(() => {
      try {
        glPresentAll();
      } catch (_) {
      }
    }, 16);
  }

  aboutToDisappear(): void {
    this.stopHold();
    if (this.timerId >= 0) {
      clearInterval(this.timerId);
      this.timerId = -1;
    }
    // Restore list-page system bars.
    this.setSystemBars('#FFFFFFFF', '#FF000000', '#FFFFFFFF');
  }

  private setSystemBars(statusColor: string, contentColor: string, navColor: string): void {
    try {
      const ctx: common.UIAbilityContext = getContext(this) as common.UIAbilityContext;
      window.getLastWindow(ctx).then((win: window.Window) => {
        win.setWindowSystemBarProperties({
          statusBarColor: statusColor,
          statusBarContentColor: contentColor,
          navigationBarColor: navColor,
          navigationBarContentColor: contentColor
        });
      }).catch((e: Error) => {
        hilog.error(DOMAIN, 'CoreGl', 'bars: %{public}s', JSON.stringify(e));
      });
    } catch (e) {
      hilog.error(DOMAIN, 'CoreGl', 'bars: %{public}s', JSON.stringify(e));
    }
  }

  private startHold(tick: () => void): void {
    this.stopHold();
    try {
      tick();
    } catch (_) {
    }
    this.holdTimerId = setInterval(() => {
      try {
        tick();
      } catch (_) {
      }
    }, 33);
  }

  private stopHold(): void {
    if (this.holdTimerId >= 0) {
      clearInterval(this.holdTimerId);
      this.holdTimerId = -1;
    }
  }

  private onDragTouch(event: TouchEvent): void {
    if (event.touches.length < 1) {
      return;
    }
    const x: number = event.touches[0].x;
    const y: number = event.touches[0].y;
    if (event.type === TouchType.Down) {
      this.dragging = true;
      this.lastX = x;
      this.lastY = y;
    } else if (event.type === TouchType.Move && this.dragging) {
      const dx: number = x - this.lastX;
      const dy: number = y - this.lastY;
      this.lastX = x;
      this.lastY = y;
      try {
        if (this.dragControlsLight) {
          glLightDragAll(dx, -dy);
        } else {
          glOrbitAll(dx, dy);
        }
      } catch (_) {
      }
    } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.dragging = false;
    }
  }

  private onHoldTouch(tick: () => void, event: TouchEvent): void {
    if (event.type === TouchType.Down) {
      this.startHold(tick);
    } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
      this.stopHold();
    }
  }

  @Builder
  private titleBuilder() {
    Row() {
      Text('立方体')
        .fontSize(20)
        .fontColor(Color.White)
        .fontWeight(FontWeight.Medium)
    }
    .height('100%')
    .alignItems(VerticalAlign.Center)
  }

  @Builder
  private holdChip(label: string, onTick: () => void) {
    Text(label)
      .fontSize(14)
      .fontColor(Color.White)
      .padding({ left: 16, right: 16, top: 8, bottom: 8 })
      .backgroundColor(IOS_CHIP_BG)
      .borderRadius(8)
      .onTouch((event: TouchEvent) => {
        this.onHoldTouch(onTick, event);
      })
  }

  /** Read @State directly — @Builder string params do not refresh on toggle. */
  @Builder
  private controlChip() {
    Text(this.dragControlsLight ? '控制:灯' : '控制:物体')
      .fontSize(14)
      .fontColor(Color.White)
      .padding({ left: 16, right: 16, top: 8, bottom: 8 })
      .backgroundColor(IOS_CHIP_BG)
      .borderRadius(8)
      .onClick(() => {
        this.dragControlsLight = !this.dragControlsLight;
      })
  }

  build() {
    NavDestination() {
      Stack() {
        XComponent({
          id: 'core_gl_surface',
          type: XComponentType.SURFACE,
          libraryname: 'core_napi',
          controller: this.xComponentController
        })
          .width('100%')
          .height('100%')

        Column()
          .width('100%')
          .height('100%')
          .onTouch((event: TouchEvent) => {
            this.onDragTouch(event);
          })

        Column() {
          Row({ space: 12 }) {
            this.holdChip('前进', () => {
              glDollyAll(1);
            })
            this.holdChip('后退', () => {
              glDollyAll(-1);
            })
            this.controlChip()
          }
          .width('100%')
          .justifyContent(FlexAlign.Center)
          // STACK title overlays content — clear mini title (~56).
          .padding({ top: 68 })

          Blank()
            .layoutWeight(1)
            .hitTestBehavior(HitTestMode.None)

          Text(this.hud)
            .fontSize(13)
            .fontColor(Color.White)
            .textAlign(TextAlign.Center)
            .width('100%')
            .padding({ left: 16, right: 16, bottom: 16, top: 8 })
            .hitTestBehavior(HitTestMode.None)
        }
        .width('100%')
        .height('100%')
        .hitTestBehavior(HitTestMode.Transparent)
      }
      .width('100%')
      .height('100%')
    }
    // Same as iOS UIColor(white:0, alpha:0.45) over GL.
    .title(this.titleBuilder, {
      backgroundColor: IOS_CHIP_BG,
      barStyle: BarStyle.STACK
    })
    .backButtonIcon(new SymbolGlyphModifier($r('sys.symbol.chevron_backward')).fontColor([Color.White]))
    .hideBackButton(false)
    .systemBarStyle({ statusBarContentColor: '#FFFFFFFF' })
    .backgroundColor(GL_BG)
  }
}
