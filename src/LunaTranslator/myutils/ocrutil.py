import os, importlib
from myutils.config import globalconfig, _TR
from qtsymbols import *
from myutils.commonbase import ArgsEmptyExc
from myutils.hwnd import safepixmap
from myutils.utils import stringfyerror, qimage2binary
from traceback import print_exc
import threading, gobject, winsharedutils


def imageCut(hwnd, x1, y1, x2, y2) -> QImage:
    return safepixmap(winsharedutils.crop_image(x1, y1, x2, y2, hwnd)).toImage()


_nowuseocrx = None
_nowuseocr = None
_ocrengine = None
_initlock = threading.Lock()


def ocr_end():
    global _ocrengine, _nowuseocr, _nowuseocrx
    with _initlock:
        _nowuseocr = None
        _nowuseocrx = None
        _ocrengine = None


def ocr_init():
    with _initlock:
        __ocr_init()


def __ocr_init():
    global _nowuseocr, _ocrengine, _nowuseocrx
    use = None
    for k in globalconfig["ocr"]:
        if globalconfig["ocr"][k]["use"] == True and os.path.exists(
            ("./LunaTranslator/ocrengines/" + k + ".py")
        ):
            use = k
            break
    _nowuseocrx = use
    if use is None:
        raise Exception("no engine")
    if _nowuseocr == use:
        return
    _ocrengine = None
    _nowuseocr = None
    aclass = importlib.import_module("ocrengines." + use).OCR
    _ocrengine = aclass(use)
    _nowuseocr = use


def ocr_run(qimage: QImage):
    gobject.baseobject.maybesetimage(qimage)
    if qimage.isNull():
        return "", None
    global _nowuseocrx, _ocrengine
    thisocrtype = _nowuseocrx
    try:
        ocr_init()
        thisocrtype = _ocrengine.typename
        image = qimage2binary(qimage, _ocrengine.required_image_format)
        if not image:
            return "", None
        res = _ocrengine._private_ocr(image)
        if not res:
            return "", None
        gobject.baseobject.maybesetocrresult(res)
        text = res["textonly"]
        if res["isocrtranslate"]:
            return text, "<notrans>"
        else:
            return text, None
    except Exception as e:
        if isinstance(e, ArgsEmptyExc):
            msg = str(e)
        else:
            print_exc()
            msg = stringfyerror(e)
        text = (
            (_TR(globalconfig["ocr"][thisocrtype]["name"]) if thisocrtype else "")
            + " "
            + msg
        )
        return text, "<msg_error_Origin>"
