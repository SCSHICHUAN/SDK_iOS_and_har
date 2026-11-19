import {
  glSetModelPath,
  glPresentAll,
  glOrbitAll,
  glOrbitEndAll,
  glDollyAll,
  glLightDragAll,
  glExplodeAll,
  glImplodeAll,
  glPickAll,
  glClearSelectionAll
} from '@sdk/core';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { common } from '@kit.AbilityKit';
import { fileIo as fs } from '@kit.CoreFileKit';
import { SymbolGlyphModifier } from '@kit.ArkUI';
import { display, window } from '@kit.ArkUI';

const DOMAIN: number = 0x0000;
const IOS_CHIP_BG: string = '#73000000';
/** Matches glClearColor(0.12, 0.13, 0.16) — status/nav/title same as GL, no black strips. */
const GL_BG: string = '#1E2129';
/** CAD / industrial selection blue */
const INDUSTRIAL_BLUE: string = '#337AFB';

export type BrowseAsset = 'cube' | 'su7' | 'benz';

function assetTitle(asset: BrowseAsset): string {
  if (asset === 'su7') {
    return 'SU7';
  }
  if (asset === 'benz') {
    return 'Benz';
  }
  return '立方体';
}

@Component
export struct ModelGLPage {
  @Prop asset: BrowseAsset = 'cube';
  @State private dragControlsLight: boolean = false;
  @State private exploded: boolean = false;
  @State private hud: string = '';
  @State private pickBannerText: string = '';
  @State private showPickBanner: boolean = false;
  private timerId: number = -1;
  private holdTimerId: number = -1;
  private pickHideTimerId: number = -1;
  private xComponentController: XComponentController = new XComponentController();
  private dragging: boolean = false;
  private moved: boolean = false;
  private lastX: number = 0;
  private lastY: number = 0;
  private downX: number = 0;
  private downY: number = 0;
  private density: number = 1;

  aboutToAppear(): void {
    try {
      this.density = display.getDefaultDisplaySync().densityPixels;
    } catch (_) {
      this.density = 1;
    }
    this.setSystemBars('#00000000', '#FFFFFFFF', GL_BG);
    this.loadModel();
    this.timerId = setInterval(() => {
      try {
        glPresentAll();
      } catch (_) {
      }
    }, 16);
  }

  aboutToDisappear(): void {
    this.stopHold();
    this.clearPickBannerTimer();
    if (this.timerId >= 0) {
      clearInterval(this.timerId);
      this.timerId = -1;
    }
    this.setSystemBars('#FFFFFFFF', '#FF000000', '#FFFFFFFF');
  }

  private loadModel(): void {
    try {
      const ctx: common.UIAbilityContext = getContext(this) as common.UIAbilityContext;
      if (this.asset === 'cube') {
        const names: string[] = ['cube.obj', 'cube.mtl', 'box.png'];
        for (let i = 0; i < names.length; ++i) {
          const name: string = names[i];
          this.copyRawToFiles(ctx, `models/${name}`, `${ctx.filesDir}/${name}`);
        }
        glSetModelPath(`${ctx.filesDir}/cube.obj`);
        this.hud = 'loaded cube — tap 选件 · 爆炸拆解';
      } else if (this.asset === 'su7') {
        const dir: string = `${ctx.filesDir}/models/su7`;
        this.ensureDir(dir);
        const dest: string = `${dir}/su7.glb`;
        this.copyRawToFiles(ctx, 'models/su7.glb', dest);
        glSetModelPath(dest);
        this.hud = 'Documents SU7 — tap 选件 · 爆炸拆解';
      } else {
        const dir: string = `${ctx.filesDir}/models/benz`;
        this.ensureDir(dir);
        const dest: string = `${dir}/benz_amg.glb`;
        this.copyRawToFiles(ctx, 'models/benz_amg.glb', dest);
        glSetModelPath(dest);
        this.hud = 'Documents Benz — tap 选件 · 爆炸拆解';
      }
    } catch (err) {
      const msg: string = err instanceof Error ? err.message : JSON.stringify(err);
      this.hud = `load failed: ${msg}`;
      hilog.error(DOMAIN, 'CoreGl', 'model: %{public}s', msg);
    }
  }

  private ensureDir(path: string): void {
    try {
      if (!fs.accessSync(path)) {
        fs.mkdirSync(path, true);
      }
    } catch (_) {
      try {
        fs.mkdirSync(path, true);
      } catch (__) {
      }
    }
  }

  private copyRawToFiles(ctx: common.UIAbilityContext, rawPath: string, outPath: string): void {
    const raw: Uint8Array = ctx.resourceManager.getRawFileContentSync(rawPath);
    try {
      if (fs.accessSync(outPath)) {
        const st = fs.statSync(outPath);
        if (st.size === raw.byteLength) {
          return;
        }
      }
    } catch (_) {
    }
    const ab: ArrayBuffer = raw.buffer.slice(raw.byteOffset, raw.byteOffset + raw.byteLength);
    const file = fs.openSync(outPath, fs.OpenMode.CREATE | fs.OpenMode.TRUNC | fs.OpenMode.WRITE_ONLY);
    fs.writeSync(file.fd, ab);
    fs.closeSync(file);
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

  private clearPickBannerTimer(): void {
    if (this.pickHideTimerId >= 0) {
      clearTimeout(this.pickHideTimerId);
      this.pickHideTimerId = -1;
    }
  }

  /** Each tap initializes a new banner object (2s, industrial blue). */
  private presentPickBanner(meshId: number): void {
    this.clearPickBannerTimer();
    if (meshId < 0) {
      this.showPickBanner = false;
      this.pickBannerText = '';
      try {
        glClearSelectionAll();
      } catch (_) {
      }
      return;
    }
    this.pickBannerText = `  配件ID：${meshId}  `;
    this.showPickBanner = true;
    this.pickHideTimerId = setTimeout(() => {
      this.showPickBanner = false;
      this.pickBannerText = '';
      this.pickHideTimerId = -1;
    }, 2000);
  }

  private onDragTouch(event: TouchEvent): void {
    if (event.touches.length < 1 && event.type !== TouchType.Up && event.type !== TouchType.Cancel) {
      return;
    }
    const x: number = event.touches.length > 0 ? event.touches[0].x : this.lastX;
    const y: number = event.touches.length > 0 ? event.touches[0].y : this.lastY;
    if (event.type === TouchType.Down) {
      this.dragging = true;
      this.moved = false;
      this.lastX = x;
      this.lastY = y;
      this.downX = x;
      this.downY = y;
    } else if (event.type === TouchType.Move && this.dragging) {
      const dx: number = x - this.lastX;
      const dy: number = y - this.lastY;
      this.lastX = x;
      this.lastY = y;
      if (Math.abs(x - this.downX) + Math.abs(y - this.downY) > 8) {
        this.moved = true;
      }
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
      if (!this.moved && !this.dragControlsLight) {
        // vp → px（与 glViewport 一致）
        const sx: number = this.downX * this.density;
        const sy: number = this.downY * this.density;
        try {
          const hit: number = glPickAll(sx, sy);
          this.presentPickBanner(hit);
        } catch (_) {
        }
      } else if (!this.dragControlsLight) {
        try {
          glOrbitEndAll();
        } catch (_) {
        }
      }
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
      Text(assetTitle(this.asset))
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
      .padding({ left: 14, right: 14, top: 8, bottom: 8 })
      .backgroundColor(IOS_CHIP_BG)
      .borderRadius(8)
      .onTouch((event: TouchEvent) => {
        this.onHoldTouch(onTick, event);
      })
  }

  @Builder
  private controlChip() {
    Text(this.dragControlsLight ? '控制:灯' : '控制:物体')
      .fontSize(14)
      .fontColor(Color.White)
      .padding({ left: 14, right: 14, top: 8, bottom: 8 })
      .backgroundColor(IOS_CHIP_BG)
      .borderRadius(8)
      .onClick(() => {
        this.dragControlsLight = !this.dragControlsLight;
      })
  }

  @Builder
  private explodeChip() {
    Text(this.exploded ? '复位' : '爆炸')
      .fontSize(14)
      .fontColor(Color.White)
      .padding({ left: 14, right: 14, top: 8, bottom: 8 })
      .backgroundColor(this.exploded ? '#48FFFFFF' : IOS_CHIP_BG)
      .borderRadius(8)
      .onClick(() => {
        this.exploded = !this.exploded;
        try {
          if (this.exploded) {
            glExplodeAll(1);
          } else {
            glImplodeAll();
          }
        } catch (_) {
        }
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
          Row({ space: 8 }) {
            this.holdChip('前进', () => {
              glDollyAll(1);
            })
            this.holdChip('后退', () => {
              glDollyAll(-1);
            })
            this.controlChip()
            this.explodeChip()
          }
          .width('100%')
          .justifyContent(FlexAlign.Center)
          .padding({ top: 68 })

          Blank()
            .layoutWeight(1)
            .hitTestBehavior(HitTestMode.None)

          if (this.showPickBanner) {
            Text(this.pickBannerText)
              .fontSize(16)
              .fontWeight(FontWeight.Medium)
              .fontColor(Color.White)
              .padding({ left: 16, right: 16, top: 10, bottom: 10 })
              .backgroundColor(INDUSTRIAL_BLUE)
              .borderRadius(10)
              .margin({ bottom: 80 })
              .hitTestBehavior(HitTestMode.None)
          }

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
