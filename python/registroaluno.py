import sys
import json
import hashlib
import os
import time
import shutil
import getpass

sys.stdout.reconfigure(encoding='utf-8')

# CORREÇÃO: Lógica simplificada para determinar BASE_DIR
if getattr(sys, 'frozen', False):
    # Executável compilado - usar diretório do executável
    BASE_DIR = os.path.dirname(sys.executable)
else:
    # Script Python - usar diretório do script
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# VERIFICAÇÃO: Se já estamos em python_embedded, usar este diretório
current_dir_name = os.path.basename(BASE_DIR)
if current_dir_name == 'python_embedded':
    # Já estamos na pasta python_embedded - usar como BASE_DIR
    PYTHON_EMBEDDED_DIR = BASE_DIR
else:
    # Estamos fora - apontar para python_embedded
    PYTHON_EMBEDDED_DIR = os.path.join(BASE_DIR, "python_embedded")

# Garante que a pasta existe
os.makedirs(PYTHON_EMBEDDED_DIR, exist_ok=True)

# DEBUG: Verificar caminhos
print(f"DEBUG: BASE_DIR = {BASE_DIR}")
print(f"DEBUG: PYTHON_EMBEDDED_DIR = {PYTHON_EMBEDDED_DIR}")
print(f"DEBUG: Current dir name = {current_dir_name}")

# DEFINIR caminhos absolutos para os arquivos JSON
USER_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "users.json")
ALUNOS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "alunos.json")
MATRICULAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "matriculas.json")

def checar_largura():
    try:
        return shutil.get_terminal_size().columns
    except (AttributeError, OSError):
        return 80

def linha(simbolo='='):
    largura = checar_largura()
    print(simbolo * largura)

def clear():
    os.system("cls" if os.name == "nt" else "clear")

def sleep(seconds=2):
    time.sleep(seconds)

def carregar_alunos():
    try:
        with open(ALUNOS_FILE, "r", encoding='utf-8') as f:  # Usar ALUNOS_FILE
            return json.load(f)
    except FileNotFoundError:
        with open(ALUNOS_FILE, "w", encoding='utf-8') as f:  # Criar em python_embedded
            json.dump({}, f, indent=4, ensure_ascii=False)
        return {}

def carregar_matricula():
    try:
        with open(MATRICULAS_FILE, "r", encoding='utf-8') as f:  # Usar MATRICULAS_FILE
            return json.load(f)
    except FileNotFoundError:
        with open(MATRICULAS_FILE, "w", encoding='utf-8') as f:  # Criar em python_embedded
            json.dump({}, f, indent=4, ensure_ascii=False)
        return {}

def salvar_alunos(alunos):
    with open(ALUNOS_FILE, "w", encoding='utf-8') as f:  # Sempre salvar em python_embedded
        json.dump(alunos, f, indent=4, ensure_ascii=False)

def registrar():
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

def login_aluno():
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
    

