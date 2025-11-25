import json
import os

# Caminho base do diretório deste arquivo e caminho do arquivo de atividades
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
ATIVIDADES_FILE = os.path.join(BASE_DIR, "atividades.json")

# Carrega um JSON do caminho especificado. Se o arquivo não existir, cria um novo
# com a chave padrão (se fornecida) e retorna o conteúdo.
def carregar_json(file_path, default_key=None):
    try:
        with open(file_path, "r", encoding='utf-8') as f:
            data = json.load(f)
        if default_key and default_key not in data:
            data[default_key] = []
        return data
    except FileNotFoundError:
        default = {default_key: []} if default_key else {}
        with open(file_path, "w", encoding='utf-8') as f:
            json.dump(default, f, indent=4, ensure_ascii=False)
        return default
    except json.JSONDecodeError:
        return {default_key: []} if default_key else {}

# Lista no console as submissões da atividade identificada por atividade_id.
def listar_submissoes(atividade_id, turma_id=None):
    data = carregar_json(ATIVIDADES_FILE, "atividades")
    atividades = data.get("atividades", [])
    
    # Procurar a atividade com filtro por turma_id se fornecido
    atividade = None
    for a in atividades:
        if a.get("atividade_id") == atividade_id:
            if turma_id is None or a.get("turma_id") == turma_id:
                atividade = a
                break
    
    if not atividade:
        if turma_id:
            print(f"\nAtividade ID {atividade_id} não encontrada na turma {turma_id}.")
        else:
            print(f"\nAtividade ID {atividade_id} não encontrada.")
        return

    submissoes = atividade.get("submissoes", [])
    if not submissoes:
        print(f"\nNenhuma submissão para a atividade ID {atividade_id}.")
        return

    print(f"\n=== Submissões da Atividade ID {atividade_id} ===")
    print(f"Descrição: {atividade.get('descricao', 'Sem descrição')}")
    print(f"Turma ID: {atividade.get('turma_id', 'N/A')}")
    print("-" * 50)
    
    for i, sub in enumerate(submissoes, 1):
        matricula = sub.get("aluno_id", "Desconhecida")
        caminho_arquivo = sub.get("arquivo", "")
        nome_arquivo = os.path.basename(caminho_arquivo) if caminho_arquivo else "N/A"
        data_envio = sub.get("data_envio", "N/A")
        print(f"{i}. Matrícula: {matricula}")
        print(f"    Arquivo: {nome_arquivo}")
        print(f"    Data: {data_envio}")
        print(f"    Caminho: {caminho_arquivo}")
        print("-" * 50)

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        try:
            aid = int(sys.argv[1])
            turma_id = int(sys.argv[2]) if len(sys.argv) > 2 else None
            listar_submissoes(aid, turma_id)
        except Exception as e:
            print(f"Erro: {e}")
    else:
        print("Uso: python listar_submissoes_module.py <atividade_id> [turma_id]")