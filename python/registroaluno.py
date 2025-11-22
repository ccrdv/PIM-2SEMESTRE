import sys
import json
import hashlib
import os
import time
import shutil
import getpass

sys.stdout.reconfigure(encoding='utf-8')

# Determina o diretório base do script ou do executável
if getattr(sys, 'frozen', False):
    BASE_DIR = os.path.dirname(sys.executable)
else:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Define o diretório python_embedded (usa BASE_DIR se já estiver dentro)
current_dir_name = os.path.basename(BASE_DIR)
if current_dir_name == 'python_embedded':
    PYTHON_EMBEDDED_DIR = BASE_DIR
else:
    PYTHON_EMBEDDED_DIR = os.path.join(BASE_DIR, "python_embedded")

# Garante que a pasta python_embedded exista
os.makedirs(PYTHON_EMBEDDED_DIR, exist_ok=True)

# Caminhos absolutos para os arquivos JSON usados pelo programa
USER_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "users.json")
ALUNOS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "alunos.json")
MATRICULAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "matriculas.json")

# Retorna a largura do terminal ou 80 como fallback.
def checar_largura():
    """Retorna a largura do terminal ou 80 como fallback."""
    try:
        return shutil.get_terminal_size().columns
    except (AttributeError, OSError):
        return 80

# Imprime uma linha horizontal usando o símbolo informado conforme a largura do terminal.
def linha(simbolo='='):
    """Imprime uma linha horizontal usando o símbolo informado conforme a largura do terminal."""
    largura = checar_largura()
    print(simbolo * largura)

# Limpa a tela do terminal (compatível Windows/Linux).
def clear():
    """Limpa a tela do terminal (compatível Windows/Linux)."""
    os.system("cls" if os.name == "nt" else "clear")

# Pausa a execução por um número de segundos (padrão 2).
def sleep(seconds=2):
    """Pausa a execução por um número de segundos (padrão 2)."""
    time.sleep(seconds)

# Carrega o arquivo alunos.json; cria um arquivo vazio se não existir e retorna o dicionário.
def carregar_alunos():
    """Carrega o arquivo alunos.json; cria um arquivo vazio se não existir e retorna o dicionário."""
    try:
        with open(ALUNOS_FILE, "r", encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        with open(ALUNOS_FILE, "w", encoding='utf-8') as f:
            json.dump({}, f, indent=4, ensure_ascii=False)
        return {}

# Carrega o arquivo matriculas.json; cria um arquivo vazio se não existir e retorna o dicionário.
def carregar_matricula():
    """Carrega o arquivo matriculas.json; cria um arquivo vazio se não existir e retorna o dicionário."""
    try:
        with open(MATRICULAS_FILE, "r", encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        with open(MATRICULAS_FILE, "w", encoding='utf-8') as f:
            json.dump({}, f, indent=4, ensure_ascii=False)
        return {}

# Salva o dicionário de alunos no arquivo alunos.json.
def salvar_alunos(alunos):
    """Salva o dicionário de alunos no arquivo alunos.json."""
    with open(ALUNOS_FILE, "w", encoding='utf-8') as f:
        json.dump(alunos, f, indent=4, ensure_ascii=False)

# Realiza o fluxo de registro de um aluno: valida matrícula, cria usuário e salva senha hash.
def registrar():
    """Realiza o fluxo de registro de um aluno: valida matrícula, cria usuário e salva senha hash."""
    clear()
    linha()
    print("=== Registro de Aluno ===")
    linha()
    matriculaaluno = input("Digite sua matrícula: ").strip()
    
    alunos = carregar_alunos()
    for user in alunos:
        if alunos[user]["matricula"] == matriculaaluno:
            linha()
            print("\033[31mEssa matrícula já possui um registro.\033[m")
            linha()
            sleep()
            return False
    
    matricula = carregar_matricula()
    if matriculaaluno in matricula:
        linha()
        print("Matrícula encontrada com sucesso!")
        print(f"Nome: {matricula[matriculaaluno]['nome'].title()}")
        print("Complete seu registro personalizado...")
        linha()
        sleep()
        clear()
    else:
        linha()
        print("\033[31mMatrícula não encontrada.\033[m")
        linha()
        sleep()
        return False
    
    username = input("\033[1mDigite seu nome de usuário ou [Enter] para cancelar: \033[m").lower().strip()
    if username == "":
        linha()
        print("\033[36mRegistro cancelado.\033[m")
        linha()
        sleep()
        return False
    if username in alunos:
        linha()
        print("\033[31mUsuário já existe. Tente outro nome de usuário.\033[m")
        linha()
        sleep()
        return False
    
    password = getpass.getpass("\033[37mDigite sua senha: \033[m")
    confirm_password = getpass.getpass("\033[37mConfirme sua senha: \033[m")
    if password == confirm_password:
        hashed_password = hashlib.sha256(password.encode()).hexdigest()
        alunos[username] = {
            "password": hashed_password,
            "matricula": matriculaaluno
        }
        salvar_alunos(alunos)
        linha()
        print(f"\033[32mUsuário {username} registrado com sucesso!\033[m")
        linha()
        sleep()
        return True
    else:
        linha()
        print("\033[31mAs senhas não são iguais.\033[m")
        linha()
        sleep()
        return False

# Realiza o fluxo de login: verifica usuário e valida senha (retorna matrícula se bem-sucedido).
def login_aluno():
    """Realiza o fluxo de login: verifica usuário e valida senha (retorna matrícula se bem-sucedido)."""
    clear()
    linha()
    print("=== Login de Aluno ===")
    linha()
    username = input("\033[1;37mDigite seu nome de usuário ou [Enter] para cancelar: \033[m").strip().lower()
    if username == "":
        linha()
        print("\033[36mLogin cancelado.\033[m")
        linha()
        sleep()
        return False
    
    alunos = carregar_alunos()
    if username not in alunos:
        linha()
        print("\033[31mUsuário não encontrado.\033[m")
        linha()
        sleep()
        return False
    
    password = getpass.getpass("\033[37mDigite sua senha: \033[m")
    hashed_password = hashlib.sha256(password.encode()).hexdigest()
    if alunos[username]["password"] == hashed_password:
        linha()
        print("\033[32mLogin realizado com sucesso!\033[m")
        linha()
        sleep()
        return alunos[username]["matricula"]
    else:
        linha()
        print("\033[31mSenha inválida.\033[m")
        linha()
        sleep()
        return False
