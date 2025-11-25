import json
import os
from datetime import datetime
from collections import defaultdict


PASTA_RELATORIOS = r"Z:\python_embedded\relatorios"
os.makedirs(PASTA_RELATORIOS, exist_ok=True)

def caminho_relatorio(nome):
    return os.path.join(PASTA_RELATORIOS, nome)

def carregar_json(arquivo):
    caminho = os.path.join(r"Z:\python_embedded", arquivo)
    if not os.path.exists(caminho):
        return {}
    with open(caminho, "r", encoding="utf-8") as f:
        return json.load(f)


def boletim_turma(turma_id):
    turmas = carregar_json("turmas.json").get("turmas", [])
    notas_lista = carregar_json("notas.json").get("notas", [])
    matriculas = carregar_json("matriculas.json")

    turma = next((t for t in turmas if t["id"] == turma_id), None)
    if not turma:
        print("Turma não encontrada!")
        return False

    nome_turma = turma["nome_turma"].replace(" ", "_").upper()
    arquivo = caminho_relatorio(f"BOLETIM_{nome_turma}_{datetime.now().strftime('%Y%m%d')}.txt")

    linhas = [
        f"BOLETIM OFICIAL - {turma['nome_turma'].upper()}",
        f"Professor(a): {turma['professor']['nome'].title()}",
        f"Gerado em: {datetime.now().strftime('%d/%m/%Y às %H:%M')}",
        "="*80,
        ""
    ]

    for aluno in turma["alunos"]:
        mat = aluno["matricula"]
        nome = matriculas.get(mat, {}).get("nome", "Desconhecido").title()
        notas_aluno = [n for n in notas_lista if n.get("matricula") == mat and n.get("turma_id") == turma_id]
        valores = [n["nota"] for n in notas_aluno]
        media = round(sum(valores)/len(valores), 2) if valores else 0.0

        linhas.append(f"{aluno['numero']:2}. {nome}")
        linhas.append(f"    Matrícula: {mat} | Média Final: {media}")
        if notas_aluno:
            for n in notas_aluno:
                desc = n.get("atividade_descricao", f"Atividade {n['atividade_id']}")
                linhas.append(f"     • {desc}: {n['nota']}")
        else:
            linhas.append("     • Nenhuma nota lançada")
        linhas.append("")

    with open(arquivo, "w", encoding="utf-8") as f:
        f.write("\n".join(linhas))

    print(f"\nBOLETIM GERADO COM SUCESSO!\nArquivo: {os.path.basename(arquivo)}\n")
    os.startfile(arquivo) 
    return True

def frequencia_turma(turma_id):
    presencas = carregar_json("presencas.json").get("presencas", [])
    turmas = carregar_json("turmas.json").get("turmas", [])
    matriculas = carregar_json("matriculas.json")

    turma = next((t for t in turmas if t["id"] == turma_id), None)
    if not turma:
        print("Turma não encontrada!")
        return False

    nome_turma = turma["nome_turma"].replace(" ", "_").upper()
    arquivo = caminho_relatorio(f"FREQUENCIA_{nome_turma}_{datetime.now().strftime('%Y%m%d')}.txt")

    total_aulas = len([p for p in presencas if p.get("turma_id") == turma_id])
    freq = defaultdict(lambda: {"presente": 0, "total": 0})

    for p in presencas:
        if p.get("turma_id") != turma_id: continue
        for pres in p.get("presencas", []):
            mat = pres["matricula"]
            if any(a["matricula"] == mat for a in turma["alunos"]):
                freq[mat]["total"] += 1
                if pres["presente"]:
                    freq[mat]["presente"] += 1

    linhas = [
        f"FREQUÊNCIA - {turma['nome_turma'].upper()}",
        f"Total de aulas registradas: {total_aulas}",
        f"Gerado em: {datetime.now().strftime('%d/%m/%Y às %H:%M')}",
        "="*80,
        ""
    ]

    for aluno in turma["alunos"]:
        mat = aluno["matricula"]
        nome = matriculas.get(mat, {}).get("nome", "Desconhecido").title()
        dados = freq[mat]
        perc = round(dados["presente"]/dados["total"]*100, 1) if dados["total"] > 0 else 0
        linhas.append(f"{aluno['numero']:2}. {nome}")
        linhas.append(f"    Matrícula: {mat} | {dados['presente']}/{dados['total']} aulas | {perc}% de frequência")
        linhas.append("")

    with open(arquivo, "w", encoding="utf-8") as f:
        f.write("\n".join(linhas))

    print(f"\nRELATÓRIO DE FREQUÊNCIA GERADO!\nArquivo: {os.path.basename(arquivo)}\n")
    os.startfile(arquivo)
    return True

def relatorio_geral():
    turmas = len(carregar_json("turmas.json").get("turmas", []))
    alunos = len(carregar_json("matriculas.json"))
    professores = len([u for u in carregar_json("users.json").get("users", []) if u.get("role") == "professor"])
    atividades = len(carregar_json("atividades.json").get("atividades", []))
    anuncios = len(carregar_json("anuncios.json").get("anuncios", []))

    arquivo = caminho_relatorio(f"RELATORIO_GERAL_{datetime.now().strftime('%Y%m%d_%H%M')}.txt")

    linhas = [
        "RELATÓRIO GERAL DO SISTEMA",
        f"Gerado em: {datetime.now().strftime('%d/%m/%Y às %H:%M')}",
        "="*60,
        f"Total de Turmas Ativas:        {turmas}",
        f"Total de Alunos Matriculados:  {alunos}",
        f"Total de Professores:          {professores}",
        f"Atividades Criadas:            {atividades}",
        f"Anúncios Postados:             {anuncios}",
        "",
        "Sistema em pleno funcionamento!",
        "Todos os dados estão sincronizados via rede."
    ]

    with open(arquivo, "w", encoding="utf-8") as f:
        f.write("\n".join(linhas))

    print(f"\nRELATÓRIO GERAL GERADO E ABERTO!\n")
    os.startfile(arquivo)