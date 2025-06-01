import sys, os

os.chdir(os.path.dirname(__file__))
sys.path.insert(0, "./LunaTranslator")
import importlib
TS=importlib.import_module('translator.chatgpt-3rd-party').TS
from language import Languages


class TS1(TS):

    @property
    def srclang(self):
        return "zh"

    @property
    def tgtlang_1(self):
        return self.xxxxx

    @tgtlang_1.setter
    def tgtlang_1(self, _):
        self.xxxxx = _


if __name__ == "__main__":

    import os, json

    f = "zh.json"
    with open("./files/lang/" + f, "r", encoding="utf8") as ff:
        js = ff.read()
        js = json.loads(js)
    needpop = []
    for k in js:
        kk = False
        try:
            k.encode("ascii")
            print(k)
            kk = True
        except:
            pass
        if k not in js or kk:
            needpop.append(k)
    for k in needpop:
        js.pop(k)
    with open(f"./files/lang/" + f, "w", encoding="utf8") as ff:
        ff.write(json.dumps(js, ensure_ascii=False, sort_keys=False, indent=4))

    for kk in os.listdir("./files/lang"):
        if kk == "zh.json":
            continue
        with open(f"./files/lang/{kk}", "r", encoding="utf8") as ff:

            jsen = json.loads(ff.read())

        needpop = []
        for k in jsen:
            if k not in js:
                needpop.append(k)
        print(kk, needpop)
        for k in needpop:
            jsen.pop(k)
        with open(f"./files/lang/{kk}", "w", encoding="utf8") as ff:
            ff.write(json.dumps(jsen, ensure_ascii=False, sort_keys=False, indent=4))
        a = TS1("chatgpt-3rd-party")
        for k in js:

            if k not in jsen or jsen[k] == "":
                a.tgtlang_1 = Languages.fromcode(kk.split(".")[0])
                print(list(a.translate(k)))
                jsen[k] = list(a.translate(k))[0]
                print(k, jsen[k])
                with open(f"./files/lang/{kk}", "w", encoding="utf8") as ff:
                    ff.write(
                        json.dumps(jsen, ensure_ascii=False, sort_keys=False, indent=4)
                    )

    os._exit(0)
