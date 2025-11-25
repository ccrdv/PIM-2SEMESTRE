import sys
import json
import os
import time
import shutil
import getpass
import datetime

sys.stdout.reconfigure(encoding='utf-8')

# Determina o diretório base (script ou executável) e define a pasta python_embedded
if getattr(sys, 'frozen', False):
    BASE_DIR = os.path.dirname(sys.executable)
else:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))

current_dir_name = os.path.basename(BASE_DIR)
if current_dir_name == 'python_embedded':
    PYTHON_EMBEDDED_DIR = BASE_DIR
else:
    PYTHON_EMBEDDED_DIR = os.path.join(BASE_DIR, "python_embedded")

os.makedirs(PYTHON_EMBEDDED_DIR, exist_ok=True)

# Caminhos absolutos para os arquivos JSON
USER_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "users.json")
ALUNOS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "alunos.json")
MATRICULAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "matriculas.json")
TURMAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "turmas.json")
ATIVIDADES_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "atividades.json")
PRESENCAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "presencas.json")
NOTAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "notas.json")

# Retorna a largura do terminal (ou 80 se não disponível)
def checar_largura():
    try:
        return shutil.get_terminal_size().columns
    except (AttributeError, OSError):
        return 80

# Imprime uma linha com o símbolo repetido até a largura do terminal
def linha(simbolo='='):
    largura = checar_largura()
    print(simbolo * largura)

# Limpa a tela do terminal
def clear():
    os.system("cls" if os.name == "nt" else "clear")

# Pausa a execução pelo número de segundos informado
def sleep(seconds=2):
    time.sleep(seconds)

# Carrega um arquivo JSON, criando-o com uma chave padrão se não existir
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

# Salva dados em formato JSON no caminho informado
def salvar_json(file_path, data):
    with open(file_path, "w", encoding='utf-8') as f:
        json.dump(data, f, indent=4, ensure_ascii=False)

# Retorna a lista de usuários com papel 'professor'
def carregar_professores():
    data = carregar_json(USER_FILE, "users")
    return [user for user in data.get("users", []) if user.get("role") == "professor"]

# Exibe a lista de professores cadastrados
def listar_professores():
    professores = carregar_professores()
    
    clear()
    linha()
    print("=== Professores Cadastrados ===")
    linha()
    
    if not professores:
        print("Nenhum professor cadastrado no sistema.")
    else:
        for i, prof in enumerate(professores, 1):
            print(f"{i}. {prof.get('nome_completo', 'N/A')}")
            print(f"   Usuário: {prof.get('username', 'N/A')}")
            print(f"   Documento: {prof.get('documento', 'N/A')}")
            print(f"   Telefone: {prof.get('telefone', 'N/A')}")
            linha('-')
    
    linha()
    input("Pressione Enter para continuar...")

# Retorna as turmas associadas a um professor pelo nome de usuário
def get_turmas_professor(username):
    data = carregar_json(TURMAS_FILE, "turmas")
    return [turma for turma in data["turmas"] if turma.get("professor", {}).get("username") == username]

# Registra presença dos alunos de uma turma para o professor informado
def registrar_presenca(professor_username):
    turmas = get_turmas_professor(professor_username)
    if not turmas:
        print("\033[31mNenhuma turma assignada a você.\033[m")
        sleep()
        return

    clear()
    linha()
    print("=== Registrar Presença ===")
    linha()
    for i, turma in enumerate(turmas, 1):
        print(f"{i}. {turma['nome_turma']} (ID: {turma['id']})")
    linha()
    escolha = input("Selecione a turma (número) ou Enter para cancelar: ").strip()
    if not escolha:
        return
    try:
        index = int(escolha) - 1
        turma = turmas[index]
    except:
        print("\033[31mSeleção inválida.\033[m")
        sleep()
        return

    alunos = turma.get("alunos", [])
    if not alunos:
        print("\033[31mNenhuma aluno na turma.\033[m")
        sleep()
        return

    data_hoje = datetime.date.today().isoformat()
    presencas_data = carregar_json(PRESENCAS_FILE, "presencas")

    print(f"Registrando presença para {turma['nome_turma']} em {data_hoje}")
    linha()
    presenca_lista = []
    for aluno in alunos:
        resp = input(f"{aluno['nome']} ({aluno['matricula']}): P (presente) / A (ausente): ").strip().upper()
        presente = resp == 'P'
        presenca_lista.append({"matricula": aluno['matricula'], "presente": presente})

    presencas_data["presencas"].append({
        "data": data_hoje,
        "turma_id": turma['id'],
        "presencas": presenca_lista
    })
    salvar_json(PRESENCAS_FILE, presencas_data)
    print("\033[32mPresença registrada!\033[m")
    sleep()

# Permite ao professor atribuir notas para atividades de uma turma
def atribuir_notas_com_ids(turma_id, atividade_id):
    """
    Recebe os IDs já validados pelo C.
    Só faz o trabalho de ler alunos, pedir notas e salvar no JSON.
    """
    turmas_data = carregar_json(TURMAS_FILE, "turmas")
    turma = None
    for t in turmas_data.get("turmas", []):
        if t["id"] == turma_id:
            turma = t
            break
    if not turma:
        print("\033[31mTurma não encontrada.\033[m")
        sleep(2)
        return

    atividades_data = carregar_json(ATIVIDADES_FILE, "atividades")
    atividade = None
    for a in atividades_data.get("atividades", []):
        if a.get("atividade_id") == atividade_id and a.get("turma_id") == turma_id:
            atividade = a
            break
    if not atividade:
        print("\033[31mAtividade não encontrada.\033[m")
        sleep(2)
        return

    alunos = turma.get("alunos", [])
    if not alunos:
        print("\033[31mNenhum aluno na turma.\033[m")
        sleep(2)
        return

    notas_data = carregar_json(NOTAS_FILE, "notas")

    clear()
    linha()
    print(f"Atribuindo notas: {atividade['descricao']}")
    print(f"Turma: {turma['nome_turma']}")
    linha()

    for aluno in alunos:
        while True:
            nota_str = input(f"{aluno['nome']} ({aluno['matricula']}): ").strip()
            if nota_str == "":
                break
            try:
                nota = float(nota_str)
                if 0 <= nota <= 10:
                    notas_data["notas"].append({
                        "turma_id": turma_id,
                        "atividade_id": atividade_id,
                        "matricula": aluno['matricula'],
                        "nota": nota
                    })
                    break
                else:
                    print("\033[31mNota deve estar entre 0 e 10.\033[m")
            except:
                print("\033[31mDigite um número válido.\033[m")

    salvar_json(NOTAS_FILE, notas_data)
    print("\033[32mNotas salvas com sucesso!\033[m")
    sleep(2)
# Autentica professor e retorna o nome completo em caso de sucesso
def login_professor():
    clear()
    linha()
    print("=== Login de Professor ===")
    linha()
    
    username = input("\033[1;37mDigite seu usuário ou [Enter] para cancelar: \033[m").strip()
    if username == "":
        linha()
        print("\033[36mLogin cancelado.\033[m")
        linha()
        sleep()
        return None
    
    professores = carregar_professores()
    professor = next((p for p in professores if p.get("username") == username), None)
    
    if not professor:
        linha()
        print("\033[31mProfessor não encontrado.\033[m")
        linha()
        sleep()
        return None
    
    password = getpass.getpass("\033[37mDigite sua senha: \033[m")
    
    if professor.get("password") == password:
        linha()
        print(f"\033[32mLogin bem-sucedido!\033[m")
        print(f"Bem-vindo, {professor.get('nome_completo', 'Professor').title()}!") 
        linha()
        sleep()
        return professor.get("username")
    else:
        linha()
        print("\033[31mSenha inválida.\033[m")
        linha()
        sleep()
        return None

# Execução de teste quando o arquivo é executado diretamente
if __name__ == "__main__":
    print("Teste do LoginProfessor")
    resultado = login_professor()
    if resultado:
        print("Login bem-sucedido!")
    else:
        print("Login falhou!")