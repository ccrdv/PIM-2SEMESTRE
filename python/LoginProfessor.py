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

def carregar_professores():
    try:
        with open(USER_FILE, "r", encoding='utf-8') as f:  # Usar USER_FILE
            data = json.load(f)
            professores = []
            for user in data.get("users", []):
                if user.get("role") == "professor":
                    professores.append(user)
            return professores
    except FileNotFoundError:
        print(f"Erro: {USER_FILE} não encontrado.")
        return []
    except json.JSONDecodeError:
        print(f"Erro: {USER_FILE} corrompido.")
        return []
    
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
        
        # Retorna o nome do professor sem exibir no console
        return professor.get('nome_completo', 'Professor')
    else:
        linha()
        print("\033[31mSenha inválida.\033[m")
        linha()
        sleep()
        return None

# Bloco para testes independentes
if __name__ == "__main__":
    print("Teste do LoginProfessor")
    resultado = login_professor()
    if resultado:
        print("Login bem-sucedido!")
    else:
        print("Login falhou!")