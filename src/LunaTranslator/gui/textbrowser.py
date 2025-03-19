from qtsymbols import *
from myutils.config import globalconfig
import importlib, copy, os, platform
from traceback import print_exc
from gui.usefulwidget import WebviewWidget
from hiraparse.basehira import basehira
from rendertext.texttype import TextType, ColorControl
from rendertext.webview import TextBrowser as WebviewTextbrowser
from rendertext.textbrowser import TextBrowser as QtTextbrowser


def checkusewhich():
    if "rendertext_using" not in globalconfig:
        v = platform.version().split(".")
        import winsharedutils

        iswin8later = tuple(int(_) for _ in platform.version().split(".")[:2]) >= (6, 2)
        webview2version = winsharedutils.detect_webview2_version()
        if iswin8later:
            if WebviewWidget.findFixedRuntime():
                # 如果手动放置，那一定选手动的，不管功能完不完整。
                globalconfig["rendertext_using"] = "webview"
            else:
                if webview2version and webview2version >= (100, 0, 0, 0):
                    # <=99的功能不完整
                    globalconfig["rendertext_using"] = "webview"
                else:
                    globalconfig["rendertext_using"] = "textbrowser"
        else:
            # win7上无边框窗口渲染有问题，所以一定不优先
            globalconfig["rendertext_using"] = "textbrowser"


class Textbrowser(QFrame):
    contentsChanged = pyqtSignal(QSize)
    dropfilecallback = pyqtSignal(str)

    def resizeEvent(self, event: QResizeEvent):
        self.textbrowser.resize(event.size())

    def _contentsChanged(self, size: QSize):
        self.contentsChanged.emit(size)

    def loadinternal(self, shoudong=False, forceReload=False):
        checkusewhich()
        __ = globalconfig["rendertext_using"]
        if (not forceReload) and (self.curr_eng == __):
            return
        self.curr_eng = __
        size = self.size()
        if self.textbrowser:
            self.textbrowser.hide()
            self.textbrowser.contentsChanged.disconnect()
            self.textbrowser.dropfilecallback.disconnect()
            self.textbrowser.deleteLater()
        try:
            tb = {"webview": WebviewTextbrowser, "textbrowser": QtTextbrowser}[__]
            self.textbrowser: QtTextbrowser = tb(self)
        except Exception as e:
            print_exc()
            if shoudong:
                WebviewWidget.showError(e)
            globalconfig["rendertext_using"] = "textbrowser"
            tb = importlib.import_module("rendertext.textbrowser").TextBrowser
            self.textbrowser = tb(self)

        if tuple(int(_) for _ in platform.version().split(".")[:2]) <= (6, 1):
            # win7不可以同时FramelessWindowHint和WA_TranslucentBackground，否则会导致无法显示
            # win8没问题
            self.window().setAttribute(
                Qt.WidgetAttribute.WA_TranslucentBackground,
                globalconfig["rendertext_using"] == "textbrowser",
            )
        self.textbrowser.move(0, 0)
        self.textbrowser.setMouseTracking(True)
        self.textbrowser.contentsChanged.connect(self._contentsChanged)
        self.textbrowser.dropfilecallback.connect(self.normdropfilepath)
        self.textbrowser.resize(size)
        self.textbrowser.show()
        self.refreshcontent()

    def normdropfilepath(self, file):
        self.dropfilecallback.emit(os.path.normpath(file))

    def refreshcontent(self):
        self.textbrowser.refreshcontent_before()
        traces = self.trace.copy()
        self.clear()
        for t, trace in traces:
            if t == 0:
                self.append(*trace)
            elif t == 1:
                self.iter_append(*trace)
        self.textbrowser.refreshcontent_after()

    def __init__(self, parent):
        super().__init__(parent)
        self.setMouseTracking(True)
        self.textbrowser = None
        self.cleared = True
        self.curr_eng = None
        self.trace = []
        self.loadinternal()

    def iter_append(
        self,
        iter_context_class,
        texttype: TextType,
        name,
        text,
        color: ColorControl,
        klass,
    ):
        self.trace.append((1, (iter_context_class, texttype, name, text, color, klass)))
        self.cleared = False
        self.textbrowser.iter_append(
            iter_context_class, texttype, name, text, color, klass
        )

    def append(self, texttype: TextType, name, text, tag, color: ColorControl, klass):
        self.trace.append(
            (
                0,
                (
                    texttype,
                    name,
                    text,
                    copy.deepcopy(tag),
                    color,
                    klass,
                ),
            )
        )
        self.cleared = False
        self.textbrowser.append(
            texttype, name, text, basehira.parseastarget(tag), color, klass
        )

    def clear(self):
        self.cleared = True
        self.trace.clear()
        self.textbrowser.clear()

    def setcolorstyle(self, _=None):
        self.textbrowser.setcolorstyle()

    def setfontstyle(self, _=None):
        self.textbrowser.setfontstyle()

    def setfontextra(self, klass):
        self.textbrowser.setfontextra(klass)

    def showhidert(self, _):
        self.textbrowser.showhidert(_)

    def showhideclick(self, _):
        self.textbrowser.showhideclick(_)

    def showhidename(self, _):
        self.textbrowser.showhidename(_)

    def showatcenter(self, _):
        self.textbrowser.showatcenter(_)

    def showhidetranslate(self, _):
        self.textbrowser.showhidetranslate(_)

    def setselectable(self, _):
        self.textbrowser.setselectable(_)

    def showhideorigin(self, _):
        self.textbrowser.showhideorigin(_)

    def showhideerror(self, _):
        self.textbrowser.showhideerror(_)

    def resetstyle(self):
        self.textbrowser.resetstyle()

    def setdisplayrank(self, type):
        self.textbrowser.setdisplayrank(type)

    def GetSelectedText(self):
        return self.textbrowser.GetSelectedText()

    def sethovercolor(self, color):
        self.textbrowser.sethovercolor(color)

    def verticalhorizontal(self, _):
        self.textbrowser.verticalhorizontal(_)