import { ModelGLPage } from './ModelGLPage';

@Entry
@Component
struct Index {
  private readonly rows: string[] = ['立方体'];
  private pageStack: NavPathStack = new NavPathStack();
  private didAutoPush: boolean = false;

  aboutToAppear(): void {
    if (this.didAutoPush) {
      return;
    }
    this.didAutoPush = true;
    this.pageStack.pushPath({ name: 'cube' });
  }

  @Builder
  private pageMap(name: string) {
    if (name === 'cube') {
      ModelGLPage();
    }
  }

  /** Root: no back, title centered + vertically centered in title bar. */
  @Builder
  private homeTitle() {
    Row() {
      Text('harmonyOSpro')
        .fontSize(20)
        .fontWeight(FontWeight.Medium)
        .fontColor(Color.Black)
    }
    .width('100%')
    .height('100%')
    .justifyContent(FlexAlign.Center)
    .alignItems(VerticalAlign.Center)
  }

  build() {
    Navigation(this.pageStack) {
      List() {
        ListItemGroup() {
          ForEach(this.rows, (title: string) => {
            ListItem() {
              Row() {
                Text(title)
                  .fontSize(17)
                  .fontColor(Color.Black)
                  .layoutWeight(1)
                SymbolGlyph($r('sys.symbol.chevron_forward'))
                  .fontSize(16)
                  .fontColor(['#C7C7CC'])
              }
              .width('100%')
              .height(48)
              .padding({ left: 16, right: 12 })
            }
            .onClick(() => {
              this.pageStack.pushPath({ name: 'cube' });
            })
          }, (title: string) => title)
        }
        .backgroundColor(Color.White)
        .borderRadius(10)
        .margin({ left: 16, right: 16, top: 12 })
      }
      .width('100%')
      .height('100%')
      .backgroundColor('#F2F2F7')
      .scrollBar(BarState.Off)
    }
    .title(this.homeTitle)
    .titleMode(NavigationTitleMode.Mini)
    .hideBackButton(true)
    .mode(NavigationMode.Stack)
    .navDestination(this.pageMap)
    .backgroundColor('#F2F2F7')
    .width('100%')
    .height('100%')
  }
}
