from qtsymbols import *
import functools
import qtawesome, winsharedutils
from myutils.config import globalconfig
from myutils.utils import get_time_stamp
from gui.usefulwidget import closeashidewindow
from gui.dynalang import LAction


class transhist(closeashidewindow):

    getnewsentencesignal = pyqtSignal(str)
    getnewtranssignal = pyqtSignal(str, str)

    def __init__(self, parent):
        super(transhist, self).__init__(parent, globalconfig["hist_geo"])
        self.setupUi()
        # self.setWindowFlags(self.windowFlags()&~Qt.WindowMinimizeButtonHint)
        self.getnewsentencesignal.connect(self.getnewsentence)
        self.getnewtranssignal.connect(self.getnewtrans)
        self.hiderawflag = False
        self.hideapiflag = False
        self.hidetime = True

        self.setWindowTitle("历史翻译")

    def setupUi(self):
        self.setWindowIcon(qtawesome.icon("fa.rotate-left"))

        textOutput = QPlainTextEdit()
        textOutput.setContextMenuPolicy(Qt.ContextMenuPolicy.CustomContextMenu)
        textOutput.customContextMenuRequested.connect(
            functools.partial(self.showmenu, textOutput)
        )
        textOutput.setUndoRedoEnabled(False)
        textOutput.setReadOnly(True)

        self.textOutput = textOutput
        self.setCentralWidget(self.textOutput)

        self.hiding = True

    def showmenu(self, tb, p):
        menu = QMenu(self)
        qingkong = LAction("清空")
        baocun = LAction("保存")
        copy = LAction("复制到剪贴板")
        hideshowraw = LAction("显示原文" if self.hiderawflag else "不显示原文")
        hideshowapi = LAction("显示api" if self.hideapiflag else "不显示api")
        hidetime = LAction("显示时间" if self.hidetime else "不显示时间")
        scrolltoend = LAction("滚动到最后")
        menu.addAction(qingkong)
        menu.addAction(baocun)
        if len(self.textOutput.textCursor().selectedText()):
            menu.addAction(copy)
        menu.addAction(scrolltoend)
        menu.addSeparator()
        menu.addAction(hideshowraw)
        menu.addAction(hideshowapi)
        menu.addAction(hidetime)

        action = menu.exec(QCursor.pos())
        if action == qingkong:
            tb.clear()
        elif action == copy:
            winsharedutils.clipboard_set(self.textOutput.textCursor().selectedText())
        elif action == baocun:
            ff = QFileDialog.getSaveFileName(self, directory="save.txt")
            if ff[0] == "":
                return
            with open(ff[0], "w", encoding="utf8") as ff:
                ff.write(tb.toPlainText())
        elif action == hideshowraw:

            self.hiderawflag = not self.hiderawflag
        elif action == hidetime:

            self.hidetime = not self.hidetime
        elif action == hideshowapi:

            self.hideapiflag = not self.hideapiflag
        elif action == scrolltoend:
            scrollbar = self.textOutput.verticalScrollBar()
            scrollbar.setValue(scrollbar.maximum())

    def getnewsentence(self, sentence):

        if self.hiderawflag:
            sentence = ""
        else:
            if not self.hidetime:
                sentence = get_time_stamp() + " " + sentence
            sentence = "\n" + sentence
        self.textOutput.appendPlainText(sentence)

    def getnewtrans(self, api, sentence):
        if not self.hideapiflag:
            sentence = api + " " + sentence
        if not self.hidetime:
            sentence = get_time_stamp() + " " + sentence

        self.textOutput.appendPlainText(sentence)
