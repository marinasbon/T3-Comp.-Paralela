#!/usr/bin/env python3
"""
T3: análise das saídas do Slurm.

Lê as linhas RESULTADO e NIVEL impressas pelos programas em resultados/*.out
e gera no diretório de destino:
  - resumo.md            tabelas: mediana, mínimo e máximo dos tempos,
                         S = T1/TP, S^delta = T1^delta/TP e E = S^delta/P
                         (E = S^delta/32 acima de 32 processos)
  - speedup.png          S^delta e E das versões original e balanceada
  - sobrecarga.png       63 a 1023 processos, yield=0 x yield=1

uso: python3 analise/analisa.py [diretório das saídas] [diretório de destino]
"""
import glob
import os
import statistics as st
import sys
from collections import defaultdict

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

ORIG = sys.argv[1] if len(sys.argv) > 1 else "resultados"
DEST = sys.argv[2] if len(sys.argv) > 2 else "resultados/analise"
os.makedirs(DEST, exist_ok=True)

THREADS_HW = 32                     # 2 nós x 16 threads (seção 5)
REPS = 10
P_ORIGINAL = [1, 3, 7, 15, 31]
P_BALANCEADA = [2, 4, 8, 16, 31, 32]
P_SOBRECARGA = [63, 127, 255, 511, 1023]
AZUL, LARANJA, VERDE, CINZA = "#2a78d6", "#eb6834", "#1baf7a", "#8a8984"


# --------------------------------------------------------------- leitura
def numero(v):
    for tipo in (int, float):
        try:
            return tipo(v)
        except ValueError:
            pass
    return v


linhas = defaultdict(list)
# A calibração usa outros tamanhos de vetor e não entra na análise.
arquivos = [a for a in sorted(glob.glob(os.path.join(ORIG, "*.out")))
            if "calibra" not in os.path.basename(a)]
if not arquivos:
    sys.exit(f"nenhum .out em {ORIG}")
for arq in arquivos:
    with open(arq, encoding="utf-8", errors="replace") as f:
        for linha in f:
            partes = linha.split()
            if partes and partes[0] in ("RESULTADO", "NIVEL"):
                reg = dict(p.split("=", 1) for p in partes[1:] if "=" in p)
                linhas[partes[0]].append({k: numero(v) for k, v in reg.items()})

res = linhas["RESULTADO"]
erros = sum(r["verificacao"] != "OK" for r in res)
if erros:
    print(f"ATENÇÃO: {erros} execuções com verificacao=ERRO", file=sys.stderr)
ns = {r["n"] for r in res}
if len(ns) != 1:
    sys.exit(f"tamanhos de vetor diferentes nas saídas: {sorted(ns)}")
N = ns.pop()

# (versao, P, delta, yield) -> tempos; e as folhas da execução
tempos, folhas = defaultdict(list), {}
for r in res:
    if r["verificacao"] == "OK":
        k = (r["versao"], r["P"], str(r["delta"]), str(r.get("yield", "-")))
        tempos[k].append(r["tempo"])
        folhas[k] = (r["folhas"], r["maior_folha"])
for k, v in sorted(tempos.items(), key=str):
    if len(v) != REPS:
        print(f"aviso: {k} tem {len(v)} repetições (esperado {REPS})",
              file=sys.stderr)


def resumo(versao, P, delta=None, yld=None):
    ks = [k for k in tempos if k[0] == versao and k[1] == P
          and (delta is None or k[2] == str(delta))
          and (yld is None or k[3] == yld)]
    if not ks:
        return None
    v = tempos[ks[0]]
    return {"med": st.median(v), "min": min(v), "max": max(v),
            "folhas": folhas[ks[0]]}


def delta_original(P):
    return (2 * N + P) // (P + 1)          # ceil(2N/(P+1)), como nos scripts


def ref_seq(versao, P):
    """T1^delta: a mesma árvore da versão paralela, em um só processo."""
    if versao.startswith("balanceada"):
        return resumo("seq_bal", P)
    return resumo("seq", 1, delta_original(P))


T1 = resumo("seq", 1, N)
if T1 is None:
    sys.exit("falta T1 (./sequencial N) nas saídas")

md = [f"# Resumo (n = {N})"]


def tabela(titulo, cab, lin):
    if not lin:
        return
    md.extend([f"\n## {titulo}\n", "| " + " | ".join(cab) + " |",
               "|" + "---|" * len(cab)])
    md.extend("| " + " | ".join(l) + " |" for l in lin)


def f(x, casas=3):
    return f"{x:.{casas}f}"


# ---------------------------------------------------------- sequenciais
lin = [["vetor inteiro (T1)", str(N), "1", str(N), f(T1["med"]), f(T1["min"]),
        f(T1["max"]), "1.00"]]
for versao, lista in (("original", P_ORIGINAL[1:] + P_SOBRECARGA),
                      ("balanceada", P_BALANCEADA)):
    for P in lista:
        r = ref_seq(versao, P)
        if r:
            d = str(delta_original(P)) if versao == "original" else "-"
            lin.append([f"{versao}, P={P}", d, str(r["folhas"][0]),
                        str(r["folhas"][1]), f(r["med"]), f(r["min"]),
                        f(r["max"]), f(T1["med"] / r["med"], 2)])
tabela("Referências sequenciais (um nó)",
       ["Árvore", "δ", "Folhas", "Maior folha", "Mediana (s)", "Mín.", "Máx.",
        "T1/T1^δ"], lin)


# ------------------------------------------------- paralelas, P <= 32
def linha_paralela(versao, P, t, base):
    ref = ref_seq(versao, P)
    sd = ref["med"] / t["med"] if ref else float("nan")
    return sd, sd / base, [versao, str(P), f"{t['folhas'][0]} ({t['folhas'][1]})",
                           f(t["med"]), f(t["min"]), f(t["max"]),
                           f(T1["med"] / t["med"], 1), f(sd, 2), f(sd / base, 2)]


lin, serie = [], defaultdict(list)
for versao, lista in (("original", P_ORIGINAL), ("balanceada", P_BALANCEADA),
                      ("balanceada_isend", P_BALANCEADA)):
    for P in lista:
        t = resumo(versao, P)
        if t:
            sd, e, l = linha_paralela(versao, P, t, P)
            serie[versao].append((P, sd, e))
            lin.append(l)
tabela("Original e balanceada (2 nós)",
       ["Versão", "P", "Folhas (maior)", "Mediana (s)", "Mín.", "Máx.", "S",
        "S^δ", "E"], lin)

# ----------------------------------------------- 63 a 1023 processos
lin, sobre = [], defaultdict(list)
for y in ("0", "1"):
    for P in P_SOBRECARGA:
        t = resumo("original", P, yld=y)
        if t:
            sd, e, l = linha_paralela("original", P, t, THREADS_HW)
            sobre[y].append((P, t, ref_seq("original", P)))
            lin.append([f"original yield={y}"] + l[1:])
tabela("Original com 63 a 1023 processos (E = S^δ/32)",
       ["Série", "P", "Folhas (maior)", "Mediana (s)", "Mín.", "Máx.", "S",
        "S^δ", "E"], lin)

# ------------------------------------- níveis da árvore da original
niveis = defaultdict(list)
for r in linhas["NIVEL"]:
    niveis[(r["P"], r["nivel"])].append(r)
lin = []
for P in (15, 31):
    for (p, nivel) in sorted(k for k in niveis if k[0] == P):
        regs = niveis[(p, nivel)]
        m = {c: st.median(x[c] for x in regs) for c in
             ("espera_pai", "conquista", "espera_filhos", "intercala", "ocupacao")}
        lin.append([str(P), str(nivel), str(regs[0]["procs"]), regs[0]["papel"],
                    f"{regs[0]['tam']:.0f}", f(m["espera_pai"]), f(m["conquista"]),
                    f(m["espera_filhos"]), f(m["intercala"]),
                    f"{100 * m['ocupacao']:.1f}%"])
tabela("Original por nível da árvore (médias do nível; mediana das repetições)",
       ["P", "Nível", "Procs", "Papel", "Tam.", "Espera pai (s)",
        "Conquista (s)", "Espera filhos (s)", "Intercala (s)",
        "Ocupação"], lin)

# ------------------------------------------------------------- gráficos
plt.rcParams.update({"font.size": 8, "axes.spines.top": False,
                     "axes.spines.right": False, "axes.grid": True,
                     "grid.color": "#e8e7e3", "legend.frameon": False,
                     "savefig.bbox": "tight"})

if serie:
    fig, (a, b) = plt.subplots(1, 2, figsize=(7, 2.6))
    xs = [1, 2, 4, 8, 16, 32]
    a.plot(xs, xs, "--", color=CINZA, lw=1, label="ideal (S^δ = P)")
    for versao, cor, mk in (("original", AZUL, "o"), ("balanceada", LARANJA, "s"),
                            ("balanceada_isend", VERDE, "^")):
        pts = serie[versao]
        if pts:
            a.plot([p[0] for p in pts], [p[1] for p in pts], mk + "-", color=cor,
                   label=versao)
            b.plot([p[0] for p in pts], [p[2] for p in pts], mk + "-", color=cor)
    for ax in (a, b):
        ax.set_xscale("log", base=2)
        ax.set_xticks(xs, [str(x) for x in xs])
        ax.set_xlabel("processos (P)")
    a.set_ylabel("S^δ = T1^δ / TP")
    b.set_ylabel("E = S^δ / P")
    b.set_ylim(0, 1.1)
    a.legend(fontsize=7)
    fig.savefig(os.path.join(DEST, "speedup.png"), dpi=160)

if sobre:
    fig, ax = plt.subplots(figsize=(3.6, 2.6))
    for y, cor, mk in (("0", AZUL, "o"), ("1", LARANJA, "s")):
        pts = sobre[y]
        if pts:
            ax.errorbar([p[0] for p in pts], [p[1]["med"] for p in pts],
                        yerr=[[p[1]["med"] - p[1]["min"] for p in pts],
                              [p[1]["max"] - p[1]["med"] for p in pts]],
                        fmt=mk + "-", color=cor, capsize=2, label=f"yield={y}")
    base = [p for p in sobre["0"] or sobre["1"] if p[2]]
    if base:
        ax.plot([p[0] for p in base], [p[2]["med"] / THREADS_HW for p in base],
                "--", color=CINZA, lw=1, label="T1^δ / 32")
    ax.set_xscale("log", base=2)
    ax.set_yscale("log")
    ax.set_xticks(P_SOBRECARGA, [str(p) for p in P_SOBRECARGA])
    ax.set_xlabel("processos (P)")
    ax.set_ylabel("TP (s)")
    ax.legend(fontsize=7)
    fig.savefig(os.path.join(DEST, "sobrecarga.png"), dpi=160)

with open(os.path.join(DEST, "resumo.md"), "w", encoding="utf-8") as out:
    out.write("\n".join(md) + "\n")
print("\n".join(md))
print(f"\narquivos gerados em {DEST}/")
