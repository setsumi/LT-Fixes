from qtsymbols import *
import platform, functools
import winsharedutils, queue, hashlib
from myutils.config import globalconfig, static_data, _TR, get_platform
from myutils.wrapper import threader, tryprint
from myutils.hwnd import getcurrexe
from myutils.utils import makehtml, getlanguse, dynamiclink
import requests, importlib
import shutil, gobject
from myutils.proxy import getproxy
import zipfile, os
import subprocess
from gui.usefulwidget import (
    D_getsimpleswitch,
    makescrollgrid,
    CollapsibleBoxWithButton,
    makesubtab_lazy,
    D_getsimplecombobox,
    makegrid,
    D_getIconButton,
)
from language import UILanguages, Languages
from gui.dynalang import LLabel, LPushButton
from gui.setting_year import yearsummary

versionchecktask = queue.Queue()


def tryqueryfromhost():

    for i, main_server in enumerate(static_data["main_server"]):
        try:
            res = requests.get(
                "{main_server}/version".format(main_server=main_server),
                verify=False,
                proxies=getproxy(),
            )
            res = res.json()
            gobject.serverindex = i
            _version = res["version"]

            return _version, res
        except:
            pass


def tryqueryfromgithub():

    res = requests.get(
        "https://api.github.com/repos/HIllya51/LunaTranslator/releases/latest",
        verify=False,
    )
    link = {
        "64": "https://github.com/HIllya51/LunaTranslator/releases/latest/download/LunaTranslator.zip",
        "32": "https://github.com/HIllya51/LunaTranslator/releases/latest/download/LunaTranslator_x86.zip",
    }
    return res.json()["tag_name"], link


def trygetupdate():

    bit = get_platform()
    try:
        version, links = tryqueryfromhost()
    except:
        try:
            version, links = tryqueryfromgithub()
        except:
            return None
    return version, links[bit], links.get("sha256", {}).get(bit, None)


def doupdate():
    if not gobject.baseobject.update_avalable:
        return
    plat = get_platform()
    if plat == "xp":
        _6432 = "32"
        bit = "_x86_winxp"
    elif plat == "32":
        bit = "_x86"
        _6432 = plat
    elif plat == "64":
        bit = ""
        _6432 = plat
    shutil.copy(
        r".\files\plugins\shareddllproxy{}.exe".format(_6432),
        gobject.getcachedir("Updater.exe"),
    )
    subprocess.Popen(
        r".\cache\Updater.exe update {} .\cache\update\LunaTranslator{} {}".format(
            int(gobject.baseobject.istriggertoupdate), bit, dynamiclink("{main_server}")
        )
    )


def updatemethod_checkalready(size, savep, sha256):
    if not os.path.exists(savep):
        return False
    if not sha256:
        return True
    with open(savep, "rb") as ff:
        newsha256 = hashlib.sha256(ff.read()).hexdigest()
        # print(newsha256, sha256)
        return newsha256 == sha256


@tryprint
def updatemethod(urls, self):
    url, sha256 = urls
    check_interrupt = lambda: not (
        globalconfig["autoupdate"] and versionchecktask.empty()
    )

    savep = gobject.getcachedir("update/" + url.split("/")[-1])
    if not savep.endswith(".zip"):
        savep += ".zip"
    r2 = requests.head(url, verify=False, proxies=getproxy())
    size = int(r2.headers["Content-Length"])
    if check_interrupt():
        return
    if updatemethod_checkalready(size, savep, sha256):
        return savep
    with open(savep, "wb") as file:
        sess = requests.session()
        r = sess.get(
            url,
            stream=True,
            verify=False,
            proxies=getproxy(),
        )
        file_size = 0
        for i in r.iter_content(chunk_size=1024 * 32):
            if check_interrupt():
                return
            if not i:
                continue
            file.write(i)
            thislen = len(i)
            file_size += thislen

            prg = int(10000 * file_size / size)
            prg100 = prg / 100
            sz = int(1000 * (int(size / 1024) / 1024)) / 1000
            self.progresssignal4.emit(
                _TR("总大小_{} MB _进度_{:0.2f}%").format(sz, prg100),
                prg,
            )

    if check_interrupt():
        return
    if updatemethod_checkalready(size, savep, sha256):
        return savep


def uncompress(self, savep):
    self.progresssignal4.emit(_TR("正在解压"), 10000)
    shutil.rmtree(gobject.getcachedir("update/LunaTranslator/"))
    with zipfile.ZipFile(savep) as zipf:
        zipf.extractall(gobject.getcachedir("update"))


@threader
def versioncheckthread(self):
    return


def createdownloadprogress(self):

    self.downloadprogress = QProgressBar(self)

    self.downloadprogress.setRange(0, 10000)
    self.downloadprogress.setVisible(False)
    self.downloadprogress.setAlignment(
        Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter
    )

    def __cb(self):
        try:
            text, val = self.downloadprogress_cache
        except:
            return
        self.downloadprogress.setValue(val)
        self.downloadprogress.setFormat(text)
        if val or text:
            self.downloadprogress.setVisible(True)

    __cb(self)
    return self.downloadprogress


def createversionlabel(self):

    self.versionlabel = LLabel()
    self.versionlabel.setOpenExternalLinks(True)
    self.versionlabel.setTextInteractionFlags(
        Qt.TextInteractionFlag.LinksAccessibleByMouse
    )
    try:
        self.versionlabel.setText(self.versionlabel_cache)
    except:
        pass
    return self.versionlabel


def versionlabelmaybesettext(self, x):
    try:
        self.versionlabel.setText(x)
    except:
        self.versionlabel_cache = x


def createimageview(self):
    lb = QLabel()
    img = QPixmap.fromImage(QImage("./files/zan.jpg"))
    img.setDevicePixelRatio(self.devicePixelRatioF())
    img = img.scaled(
        500,
        500,
        Qt.AspectRatioMode.KeepAspectRatio,
        Qt.TransformationMode.SmoothTransformation,
    )
    lb.setPixmap(img)
    return lb


def delayloadlinks(key, lay):
    sources = static_data["aboutsource"][key]
    grid = []
    for source in sources:
        __grid = []
        function = source.get("function")
        if function:
            func = getattr(
                importlib.import_module(function[0]),
                function[1],
            )
            __grid.append([(func, 0)])
        else:
            for link in source["links"]:
                __grid.append(
                    [
                        link["name"],
                        (makehtml(link["link"], link.get("vis", None)), 2, "link"),
                    ]
                    + ([link.get("about")] if link.get("about") else [])
                )
        grid.append(
            [
                (
                    dict(title=source.get("name", None), type="grid", grid=__grid),
                    0,
                    "group",
                )
            ]
        )
    w, do = makegrid(grid, delay=True)
    lay.addWidget(w)
    do()


def offlinelinks(key):
    box = CollapsibleBoxWithButton(functools.partial(delayloadlinks, key), "下载")
    return box


def updatelog():

    box = LPushButton("更新记录")
    box.clicked.connect(lambda: os.startfile(dynamiclink("{main_server}/ChangeLog")))
    return box


def setTab_about1(self, basel):

    shuominggrid = [
        ["Github", makehtml("https://github.com/setsumi/LT-Fixes")],
        [
            "使用说明",
            makehtml("{docs_server}", show="https://docs.lunatranslator.org/"),
        ],
    ]
    makescrollgrid(
        [
            [
                (
                    dict(
                        grid=shuominggrid,
                    ),
                    0,
                    "group",
                )
            ],
        ],
        basel,
    )


def setTab_about(self, basel):
    tab_widget, do = makesubtab_lazy(
        [
            "关于软件",
            "其他设置",
            # "年度总结"
        ],
        [
            functools.partial(setTab_about1, self),
            functools.partial(setTab_update, self),
            # functools.partial(yearsummary, self),
        ],
        delay=True,
    )
    basel.addWidget(tab_widget)
    do()


def changeUIlanguage(_):
    languageChangeEvent = QEvent(QEvent.Type.LanguageChange)
    QApplication.sendEvent(QApplication.instance(), languageChangeEvent)
    try:
        gobject.baseobject.textsource.setlang()
    except:
        pass


def setTab_update(self, basel):
    version = winsharedutils.queryversion(getcurrexe())
    if version is None:
        versionstring = "unknown"
    else:
        versionstring = ("v{}.{}.{}  {}").format(
            version[0], version[1], version[2], platform.architecture()[0]
        )
    inner, vis = [_.code for _ in UILanguages], [_.nativename for _ in UILanguages]
    grid2 = [
        [
            (
                dict(
                    title="版本更新",
                    type="grid",
                    grid=[
                        [
                            "当前版本",
                            versionstring,
                            "",
                            functools.partial(updatelog),
                        ],
                        [(functools.partial(createdownloadprogress, self), 0)],
                    ],
                ),
                0,
                "group",
            ),
        ],
        [
            (
                dict(
                    title="软件显示语言",
                    type="grid",
                    grid=[
                        [
                            "软件显示语言",
                            D_getsimplecombobox(
                                vis,
                                globalconfig,
                                "languageuse2",
                                callback=changeUIlanguage,
                                static=True,
                                internal=inner,
                            ),
                            D_getIconButton(
                                callback=lambda: os.startfile(
                                    os.path.abspath(
                                        "./files/lang/{}.json".format(getlanguse())
                                    )
                                ),
                            ),
                        ],
                    ],
                ),
                0,
                "group",
            ),
        ],
    ]

    makescrollgrid(grid2, basel)
