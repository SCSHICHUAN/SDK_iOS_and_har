@Entry
@Component
struct Index {
  @State private showSplash: boolean = true;   // 启动页开关
  private initDone: boolean = false;           // 初始化完成标记
  private startTime: number = 0;               // 启动时刻

  aboutToAppear(): void {
    this.startTime = Date.now();
    this.initApp();
  }

  // 模拟初始化：数据库 / 登录态 / SDK，完成后进 maybeEnter
  private initApp(): void {
    setTimeout(() => {
      this.initDone = true;
      this.maybeEnter();
    }, 300);   // 实际替换成你的异步初始化
  }

  // 双条件：初始化完成 && 展示 ≥ 1.5s，才进主界面
  private maybeEnter(): void {
    const remain = Math.max(0, 1500 - (Date.now() - this.startTime));
    setTimeout(() => {
      this.showSplash = false;
    }, remain);
  }

  build() {
    if (this.showSplash) {
      // ===== 启动页：iOS LaunchScreen 样式 =====
      Column() {
        Text('harmonyOSpro')
          .fontSize(40)
          .fontWeight(FontWeight.Bold)
          .fontColor('#1F2328')
          .letterSpacing(1)
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)      // 垂直居中
      .alignItems(HorizontalAlign.Center)    // 水平居中
      .backgroundColor('#FFFFFF')            // 纯白底
    } else {
      // ===== 主界面（占位） =====
      Column() {
        Text('主界面')
          .fontSize(20)
      }
      .width('100%')
      .height('100%')
      .justifyContent(FlexAlign.Center)
    }
  }
}
