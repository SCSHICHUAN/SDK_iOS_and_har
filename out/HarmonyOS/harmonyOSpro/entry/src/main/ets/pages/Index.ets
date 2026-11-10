import { helloWorld, helloWorldString } from '@sdk/core';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN: number = 0x0000;

@Entry
@Component
struct Index {
  @State private showSplash: boolean = true;
  @State private coreMsg: string = 'core not called';
  private initDone: boolean = false;
  private startTime: number = 0;

  aboutToAppear(): void {
    this.startTime = Date.now();
    this.initApp();
  }

  private callCore(): string {
    const ret: number = helloWorld();
    const text: string = helloWorldString();
    hilog.info(DOMAIN, 'CoreNapi', 'helloWorld => %{public}d', ret);
    hilog.info(DOMAIN, 'CoreNapi', 'helloWorldString => %{public}s', text);
    return `core_hello_world() => ${ret}\ncore_hello_world_string() => ${text}`;
  }

  private initApp(): void {
    try {
      this.coreMsg = this.callCore();
    } catch (err) {
      const msg: string = err instanceof Error ? err.message : JSON.stringify(err);
      this.coreMsg = `core call failed: ${msg}`;
      hilog.error(DOMAIN, 'CoreNapi', 'helloWorld failed: %{public}s', msg);
    }
    setTimeout(() => {
      this.initDone = true;
      this.maybeEnter();
    }, 300);
  }

  private maybeEnter(): void {
    const remain: number = Math.max(0, 1500 - (Date.now() - this.startTime));
    setTimeout(() => {
      this.showSplash = false;
    }, remain);
  }

  build() {
    if (this.showSplash) {
      Column() {
        Text('harmonyOSpro')
          .fontSize(40)
          .fontWeight(FontWeight.Bold)
          .fontColor('#1F2328')
          .letterSpacing(1)
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
      .backgroundColor('#FFFFFF')
    } else {
      Column({ space: 12 }) {
        Text('主界面')
          .fontSize(20)
        Text(this.coreMsg)
          .fontSize(16)
          .fontColor('#333333')
        Button('再调用 core')
          .onClick(() => {
            try {
              this.coreMsg = this.callCore();
            } catch (err) {
              const msg: string = err instanceof Error ? err.message : JSON.stringify(err);
              this.coreMsg = `core call failed: ${msg}`;
            }
          })
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)
      .alignItems(HorizontalAlign.Center)
    }
  }
}
