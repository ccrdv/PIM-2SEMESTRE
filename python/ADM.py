import sys
import json
import os
import time
import shutil

sys.stdout.reconfigure(encoding='utf-8')

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

USER_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "users.json")
TURMAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "turmas.json")
ALUNOS_FILE = os.path.join(PYTHON_EMBEDDIR, "alunos.json") if False else os.path.join(PYTHON_EMBEDDED_DIR, "alunos.json")
MATRICULAS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "matriculas.json")
ATIVIDADES_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "atividades.json")
ANUNCIOS_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "anuncios.json")

sys.path.insert(0, PYTHON_EMBEDDED_DIR)

try:
    import cadastrosecretaria
except ImportError as e:
    print(f"Erro ao importar cadastrosecretaria: {e}")
    print(f"PYTHON_EMBEDDED_DIR: {PYTHON_EMBEDDED_DIR}")
    print(f"Conteúdo do diretório: {os.listdir(PYTHON_EMBEDDED_DIR) if os.path.exists(PYTHON_EMBEDDED_DIR) else 'Diretório não existe'}")
    cadastrosecretaria = None

# Função: checar_largura — retorna a largura do terminal ou 80 como fallback
def checar_largura():
    try:
        return shutil.get_terminal_size().columns
    except (AttributeError, OSError):
        return 80

# Função: linha — imprime uma linha horizontal usando o símbolo especificado
def linha(simbolo='='):
    largura = checar_largura()
    print(simbolo * largura)

# Função: clear — limpa a tela do terminal conforme o sistema operacional
def clear():
    os.system("cls" if os.name == "nt" else "clear")

# Função: formatar_cpf — formata CPF de 11 dígitos como XXX.XXX.XXX-XX
def formatar_cpf(cpf):
    if len(cpf) == 11 and cpf.isdigit():
        return f"{cpf[:3]}.{cpf[3:6]}.{cpf[6:9]}-{cpf[9:]}"
    return cpf

# Função: desformatar_cpf — remove caracteres não numéricos de um CPF
def desformatar_cpf(cpf):
    return ''.join(filter(str.isdigit, cpf))

# Função: formatar_data_nascimento — formata data DDMMAAAA para DD/MM/AAAA
def formatar_data_nascimento(data):
    if len(data) == 8 and data.isdigit():
        return f"{data[:2]}/{data[2:4]}/{data[4:]}"
    return data

# Função: formatar_telefone — formata telefone de 11 dígitos como (XX) XXXXX-XXXX
def formatar_telefone(telefone):
    if len(telefone) == 11 and telefone.isdigit():
        return f"({telefone[:2]}) {telefone[2:7]}-{telefone[7:]}"
    return telefone

# Função: init_users — inicializa arquivo de usuários com administrador padrão se ausente
def init_users():
    default_users = {
        "users": [
            {
                "username": "adm",
                "password": "adm",
                "nome_completo": "Administrador Padrão",
                "documento": "123.456.789-00",
                "data_nascimento": "01/01/1980",
                "telefone": "(11) 99999-9999",
                "endereco": "Rua Padrão, 123",
                "sexo": "outro",
                "role": "admin"
            }
        ]
    }
    if not os.path.exists(USER_FILE):
        with open(USER_FILE, 'w', encoding='utf-8') as f:
            json.dump(default_users, f, indent=4, ensure_ascii=False)
        linha()
        print("\033[32mBanco de dados de usuários inicializado com administrador padrão (adm/adm)\033[m")
        linha()
        time.sleep(2)
        clear()

# Função: load_users — carrega usuários do arquivo JSON, inicializa se necessário
def load_users():
    try:
        with open(USER_FILE, 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        init_users()
        return load_users()

# Função: load_alunos — carrega e normaliza dados de alunos de diversos formatos possíveis
def load_alunos():
    """Carrega alunos do arquivo alunos.json e converte para a estrutura esperada.

    Suporta formas comuns encontradas em arquivos JSON:
    - mapeamento username -> {matricula: ...}
    - {"alunos": [ {"username":..., "matricula":..., ...}, ... ]}
    - lista direta: [ {"username":..., ...}, ... ]

    Retorna sempre {'alunos': [ {username, nome_completo, matricula, documento}, ... ]}
    """
    try:
        with open(ALUNOS_FILE, 'r', encoding='utf-8') as f:
            data = json.load(f)

        users_data = load_users()
        alunos_lista = []

        if isinstance(data, dict) and 'alunos' in data and isinstance(data['alunos'], list):
            raw_list = data['alunos']
            for item in raw_list:
                if not isinstance(item, dict):
                    continue
                username = item.get('username')
                if username is not None:
                    username = str(username).lower()
                matricula = item.get('matricula')
                if matricula is not None:
                    matricula = str(matricula).upper()
                nome = item.get('nome') or item.get('nome_completo')
                documento = item.get('documento')

                if username:
                    aluno_info = next((u for u in users_data['users'] if str(u.get('username','')).lower() == username), None)
                    if aluno_info:
                        nome = nome or aluno_info.get('nome_completo')
                        documento = documento or aluno_info.get('documento')

                alunos_lista.append({
                    'username': username,
                    'nome_completo': nome,
                    'matricula': matricula,
                    'documento': documento
                })

        elif isinstance(data, dict):
            for username, dados in data.items():
                if not isinstance(username, str):
                    continue
                uname = username.lower()
                aluno_info = next((u for u in users_data['users'] if str(u.get('username','')).lower() == uname), None)
                matricula = None
                if isinstance(dados, dict):
                    matricula = dados.get('matricula')
                    if matricula is not None:
                        matricula = str(matricula).upper()
                if aluno_info:
                    alunos_lista.append({
                        'username': uname,
                        'nome_completo': aluno_info.get('nome_completo'),
                        'matricula': matricula,
                        'documento': aluno_info.get('documento')
                    })

        elif isinstance(data, list):
            for item in data:
                if not isinstance(item, dict):
                    continue
                username = item.get('username')
                if username is not None:
                    username = str(username).lower()
                matricula = item.get('matricula')
                if matricula is not None:
                    matricula = str(matricula).upper()
                nome = item.get('nome') or item.get('nome_completo')
                documento = item.get('documento')

                if username:
                    aluno_info = next((u for u in users_data['users'] if str(u.get('username','')).lower() == username), None)
                    if aluno_info:
                        nome = nome or aluno_info.get('nome_completo')
                        documento = documento or aluno_info.get('documento')

                alunos_lista.append({
                    'username': username,
                    'nome_completo': nome,
                    'matricula': matricula,
                    'documento': documento
                })

        return {'alunos': alunos_lista}
    except FileNotFoundError:
        return {'alunos': []}
    except Exception as e:
        print(f"\033[31mErro ao carregar alunos: {e}\033[m")
        return {'alunos': []}

# Função: load_matriculas — carrega e normaliza dados de matrículas de vários formatos
def load_matriculas():
    """Carrega alunos a partir de matriculas.json e retorna no formato {'alunos': [...]}

    Espera que cada entrada contenha pelo menos: username, nome_completo (ou nome), matricula, documento.
    Se o arquivo não existir, devolve {'alunos': []}.
    """
    try:
        with open(MATRICULAS_FILE, 'r', encoding='utf-8') as f:
            data = json.load(f)

        alunos_lista = []

        if isinstance(data, dict) and 'matriculas' in data and isinstance(data['matriculas'], list):
            raw_list = data['matriculas']
        elif isinstance(data, dict) and 'alunos' in data and isinstance(data['alunos'], list):
            raw_list = data['alunos']
        elif isinstance(data, list):
            raw_list = data
        elif isinstance(data, dict):
            raw_list = []
            for key, dados in data.items():
                if isinstance(dados, dict):
                    entry = dados.copy()
                    entry.setdefault('matricula', str(key).upper())
                    entry.setdefault('username', str(key).lower())
                    raw_list.append(entry)
        else:
            raw_list = []

        for item in raw_list:
            if not isinstance(item, dict):
                continue
            username = item.get('username')
            if username is not None:
                username = str(username).lower()
            matricula = item.get('matricula')
            nome = item.get('nome') or item.get('nome_completo') or item.get('full_name')
            documento = item.get('documento') or item.get('cpf')

            alunos_lista.append({
                'username': username,
                'nome_completo': nome,
                'matricula': matricula,
                'documento': documento
            })

        return {'alunos': alunos_lista}
    except FileNotFoundError:
        return {'alunos': []}
    except Exception as e:
        print(f"\033[31mErro ao carregar matriculas: {e}\033[m")
        return {'alunos': []}

# Função: load_professores — retorna lista de usuários com role 'professor'
def load_professores():
    """Carrega todos os usuários com role 'professor'"""
    users_data = load_users()
    professores = []
    for user in users_data["users"]:
        if user["role"] == "professor":
            professores.append(user)
    return professores

# Função: save_users — persiste objeto de usuários no arquivo JSON
def save_users(users):
    with open(USER_FILE, 'w', encoding='utf-8') as f:
        json.dump(users, f, indent=4, ensure_ascii=False)

# Função: register — interface de cadastro de novo usuário
def register():
    clear()
    linha()
    print("\033[1m=== Cadastro ===\033[m")
    print("1. Administrador")
    print("2. Funcionário")
    print("3. Voltar")
    linha()
    
    choice = input("Selecione o tipo de usuário (1-3): ").strip()
    
    if choice == "3":
        clear()
        return False
    
    if choice not in ["1", "2"]:
        linha()
        print("\033[31mOpção inválida!\033[m")
        linha()
        time.sleep(2)
        clear()
        return False
    
    if choice == "2":
        clear()
        linha()
        print("\033[1m=== Tipo de Funcionário ===\033[m")
        print("1. Professor")
        print("2. Secretária")
        linha()
        sub_choice = input("Selecione o tipo de funcionário (1-2): ").strip()
        
        if sub_choice == "1":
            role = "professor"
        elif sub_choice == "2":
            role = "secretaria"
        else:
            linha()
            print("\033[31mOpção inválida!\033[m")
            linha()
            time.sleep(2)
            clear()
            return False
    else:
        role = "admin"
    
    clear()
    linha()
    print("\033[1m=== Dados do Usuário ===\033[m")
    username = input("Digite o nome de usuário: ").strip().lower()
    password = input("Digite a senha: ").strip()
    nome_completo = input("Digite o nome completo: ").lower().strip()
    cpf = input("Digite o CPF (Somente Números): ").strip()
    data_nascimento = input("Digite a data de nascimento (DDMMAAAA): ").strip()
    telefone = input("Digite o telefone (Somente Números, 11 dígitos): ").strip()
    endereco = input("Digite o endereço: ").lower().strip()
    sexo = input("Digite o sexo (M/F/Outro): ").lower().strip()
    linha()
    
    if not all([username, password, nome_completo, cpf, data_nascimento, telefone, endereco, sexo]):
        print("\033[31mTodos os campos são obrigatórios!\033[m")
        linha()
        time.sleep(2)
        clear()
        return False
    
    if len(cpf) != 11 or not cpf.isdigit():
        print("\033[31mCPF inválido! Deve conter 11 dígitos numéricos.\033[m")
        linha()
        time.sleep(2)
        clear()
        return False
    
    if len(data_nascimento) != 8 or not data_nascimento.isdigit():
        print("\033[31mData de nascimento inválida! Use o formato DDMMAAAA.\033[m")
        linha()
        time.sleep(2)
        clear()
        return False
    
    if len(telefone) != 11 or not telefone.isdigit():
        print("\033[31mTelefone inválido! Deve conter 11 dígitos numéricos.\033[m")
        linha()
        time.sleep(2)
        clear()
        return False
    
    if sexo not in ["m", "f", "outro"]:
        print("\033[31mSexo inválido! Use M, F ou Outro.\033[m")
        linha()
        time.sleep(2)
        clear()
        return False
    
    users_data = load_users()
    
    for user in users_data["users"]:
        if str(user.get("username", "")).lower() == username:
            print("\033[31mNome de usuário já existe!\033[m")
            linha()
            time.sleep(2)
            clear()
            return False
        if desformatar_cpf(user["documento"]) == cpf:
            print("\033[31mCPF já cadastrado!\033[m")
            linha()
            time.sleep(2)
            clear()
            return False
    
    cpf_formatado = formatar_cpf(cpf)
    data_nascimento_formatada = formatar_data_nascimento(data_nascimento)
    telefone_formatado = formatar_telefone(telefone)
    
    users_data["users"].append({
        "username": username,
        "password": password,
        "nome_completo": nome_completo,
        "documento": cpf_formatado,
        "data_nascimento": data_nascimento_formatada,
        "telefone": telefone_formatado,
        "endereco": endereco,
        "sexo": sexo,
        "role": role
    })
    save_users(users_data)
    print(f"\033[32mUsuário {username} ({role}) cadastrado com sucesso!\033[m")
    linha()
    time.sleep(2)
    clear()
    return True

# Função: login — autentica usuário e retorna (sucesso, role)
def login(username, password):
    username = username.strip().lower()
    users_data = load_users()
    
    for user in users_data["users"]:
        if str(user.get("username","")).lower() == username and user.get("password") == password:
            if user["role"] in ["admin", "secretaria"]:
                linha()
                print(f"\033[32mBem-vindo(a), {user['nome_completo'].title()}! ({user['role'].capitalize()})\033[m")
                linha()
                time.sleep(2)
                clear()
                return True, user["role"]
            else:
                linha()
                print("\033[31mAcesso negado! Apenas administradores e secretárias podem acessar este sistema.\033[m")
                linha()
                time.sleep(2)
                clear()
                return False, None
    
    linha()
    print("\033[31mNome de usuário ou senha inválidos!\033[m")
    linha()
    time.sleep(2)
    clear()
    return False, None

# Função: remover_usuario_por_documento — remove usuário pelo CPF formatado ou não
def remover_usuario_por_documento(documento):
    documento_desformatado = desformatar_cpf(documento)
    users_data = load_users()
    
    for i, user in enumerate(users_data["users"]):
        if desformatar_cpf(user["documento"]) == documento_desformatado:
            nome = user["nome_completo"].title()
            users_data["users"].pop(i)
            save_users(users_data)
            print(f"\033[32mUsuário {nome} removido com sucesso!\033[m")
            linha()
            time.sleep(2)
            clear()
            return True
    
    print("\033[31mCPF não encontrado!\033[m")
    linha()
    time.sleep(2)
    clear()
    return False

# Função: remover_usuario — interface para entrada de CPF e remoção de usuário
def remover_usuario():
    clear()
    linha()
    print("\033[1m=== Remover Usuário ===\033[m")
    documento = input("Digite o CPF do usuário (Somente Números): ").strip()
    linha()
    return remover_usuario_por_documento(documento)

# Função: menu_admin — interface interativa para operações de administrador
def menu_admin():
    while True:
        clear()
        linha()
        print("\033[1m=== Sistema de Gerenciamento - Administrador ===\033[m")
        print("1. Cadastrar Usuário")
        print("2. Remover Usuário")
        print("3. Criar Turma")
        print("4. Gerenciar Turmas")
        print("5. Gerenciar Matrículas")
        print("6. Sair")
        linha()
        
        choice = input("Selecione uma opção (1-6): ").strip()
        
        if choice == "1":
            register()
        elif choice == "2":
            remover_usuario()
        elif choice == "3":
            criar_turma()
        elif choice == "4":
            gerenciar_turmas()
        elif choice == "5":
            clear()
            if cadastrosecretaria:
                cadastrosecretaria.menu_principal()
        elif choice == "6":
            linha()
            print("\033[31mSaindo...\033[m")
            linha()
            time.sleep(2)
            clear()
            break
        else:
            linha()
            print("\033[31mOpção inválida! Tente novamente.\033[m")
            linha()
            time.sleep(2)
            clear()

# Função: listar_turmas — imprime todas as turmas cadastradas de forma legível
def listar_turmas():
    """Lista todas as turmas de forma formatada"""
    turmas_data = load_turmas()
    if not turmas_data["turmas"]:
        print("\033[33mNenhuma turma cadastrada!\033[m")
        return False
    
    for turma in turmas_data["turmas"]:
        linha()
        print(f"\033[1mTurma: {turma['nome_turma']} (ID: {turma['id']})\033[m")
        print(f"Professor: {turma['professor']['nome']}")
        print(f"Total de Alunos: {len(turma['alunos'])}")
        print("Alunos:")
        for aluno in turma["alunos"]:
            nome = (aluno.get('nome') or aluno.get('nome_completo') or 'N/A')
            print(f"  {aluno['numero']}. {nome}")
    return True

# Função: selecionar_turma_interativo — permite ao usuário selecionar uma turma via input
def selecionar_turma_interativo():
    """Permite selecionar uma turma de forma interativa"""
    turmas_data = load_turmas()
    if not turmas_data["turmas"]:
        print("\033[33mNenhuma turma cadastrada!\033[m")
        return None
    
    clear()
    linha()
    print("\033[1m=== Selecionar Turma ===\033[m")
    for i, turma in enumerate(turmas_data["turmas"], 1):
        print(f"{i}. {turma['nome_turma']} - {turma['professor']['nome']} ({len(turma['alunos'])} alunos)")
    
    linha()
    try:
        escolha = int(input("Selecione a turma (número): "))
        if 1 <= escolha <= len(turmas_data["turmas"]):
            return turmas_data["turmas"][escolha - 1], escolha - 1
        else:
            print("\033[31mSeleção inválida!\033[m")
            return None
    except ValueError:
        print("\033[31mDigite um número válido!\033[m")
        return None

# Função: editar_turma — menu de edição para modificar dados de uma turma específica
def editar_turma(turma, turma_index):
    """Menu de edição para uma turma específica"""
    while True:
        clear()
        linha()
        print(f"\033[1m=== Editando Turma: {turma['nome_turma']} ===\033[m")
        print(f"Professor: {turma['professor']['nome']}")
        print(f"Total de Alunos: {len(turma['alunos'])}")
        linha()
        print("1. Alterar Nome da Turma")
        print("2. Alterar Professor")
        print("3. Adicionar Aluno")
        print("4. Remover Aluno")
        print("5. Listar Alunos")
        print("6. Voltar")
        linha()
        
        opcao = input("Selecione uma opção (1-6): ").strip()
        
        if opcao == "1":
            novo_nome = input("Novo nome da turma: ").strip()
            if novo_nome:
                turmas_data = load_turmas()
                nome_existe = any(t['nome_turma'].lower() == novo_nome.lower() 
                                for i, t in enumerate(turmas_data["turmas"]) 
                                if i != turma_index)
                if nome_existe:
                    print("\033[31mJá existe uma turma com este nome!\033[m")
                    time.sleep(2)
                else:
                    turma['nome_turma'] = novo_nome
                    turmas_data["turmas"][turma_index] = turma
                    save_turmas(turmas_data)
                    print("\033[32mNome da turma atualizado com sucesso!\033[m")
                    time.sleep(2)
            else:
                print("\033[31mNome não pode ser vazio!\033[m")
                time.sleep(2)
                
        elif opcao == "2":
            professores = load_professores()
            if not professores:
                print("\033[31mNenhum professor cadastrado!\033[m")
                time.sleep(2)
                continue
                
            clear()
            linha()
            print("\033[1m=== Selecionar Novo Professor ===\033[m")
            for i, prof in enumerate(professores, 1):
                print(f"{i}. {prof['nome_completo'].title()} - CPF: {prof['documento']}")
            
            linha()
            try:
                escolha = int(input("Selecione o novo professor (número): "))
                if 1 <= escolha <= len(professores):
                    novo_professor = professores[escolha - 1]
                    turma['professor'] = {
                        'nome': novo_professor['nome_completo'].lower(),
                        'documento': novo_professor['documento'],
                        'username': novo_professor['username']
                    }
                    turmas_data = load_turmas()
                    turmas_data["turmas"][turma_index] = turma
                    save_turmas(turmas_data)
                    print(f"\033[32mProfessor alterado para: {novo_professor['nome_completo'].title()}!\033[m")
                    time.sleep(2)
                else:
                    print("\033[31mSeleção inválida!\033[m")
                    time.sleep(2)
            except ValueError:
                print("\033[31mDigite um número válido!\033[m")
                time.sleep(2)
                
        elif opcao == "3":
            alunos_data = load_matriculas()
            alunos_na_turma = [a['matricula'] for a in turma['alunos']]
            alunos_disponiveis = [a for a in alunos_data["alunos"] 
                                if a['matricula'] not in alunos_na_turma]
            
            if not alunos_disponiveis:
                print("\033[33mTodos os alunos já estão nesta turma!\033[m")
                time.sleep(2)
                continue
                
            clear()
            linha()
            print("\033[1m=== Adicionar Aluno ===\033[m")
            for i, aluno in enumerate(alunos_disponiveis, 1):
                nome = (aluno.get('nome_completo') or aluno.get('nome') or 'N/A')
                print(f"{i}. {nome.title()} ({aluno.get('matricula')})")
            
            linha()
            try:
                escolha = int(input("Selecione o aluno para adicionar (número): "))
                if 1 <= escolha <= len(alunos_disponiveis):
                    aluno_selecionado = alunos_disponiveis[escolha - 1]
                    novo_numero = len(turma['alunos']) + 1
                    turma['alunos'].append({
                        'numero': novo_numero,
                        'matricula': aluno_selecionado['matricula'],
                        'nome': aluno_selecionado['nome_completo'].lower(),
                        'documento': aluno_selecionado['documento']
                    })
                    turmas_data = load_turmas()
                    turmas_data["turmas"][turma_index] = turma
                    save_turmas(turmas_data)
                    print(f"\033[32m{aluno_selecionado['nome_completo'].title()} adicionado à turma!\033[m")
                    time.sleep(2)
                else:
                    print("\033[31mSeleção inválida!\033[m")
                    time.sleep(2)
            except ValueError:
                print("\033[31mDigite um número válido!\033[m")
                time.sleep(2)
                
        elif opcao == "4":
            if not turma['alunos']:
                print("\033[33mNão há alunos para remover!\033[m")
                time.sleep(2)
                continue
                
            clear()
            linha()
            print("\033[1m=== Remover Aluno ===\033[m")
            for aluno in turma['alunos']:
                nome = (aluno.get('nome') or aluno.get('nome_completo') or 'N/A')
                print(f"{aluno['numero']}. {nome} ({aluno.get('matricula')})")
            
            linha()
            try:
                numero_aluno = int(input("Digite o número do aluno a remover: "))
                aluno_para_remover = next((a for a in turma['alunos'] if a['numero'] == numero_aluno), None)
                if aluno_para_remover:
                    turma['alunos'] = [a for a in turma['alunos'] if a['numero'] != numero_aluno]
                    for i, aluno in enumerate(turma['alunos'], 1):
                        aluno['numero'] = i
                    
                    turmas_data = load_turmas()
                    turmas_data["turmas"][turma_index] = turma
                    save_turmas(turmas_data)
                    print(f"\033[32m{aluno_para_remover['nome']} removido da turma!\033[m")
                    time.sleep(2)
                else:
                    print("\033[31mAluno não encontrado!\033[m")
                    time.sleep(2)
            except ValueError:
                print("\033[31mDigite um número válido!\033[m")
                time.sleep(2)
                
        elif opcao == "5":
            clear()
            linha()
            print(f"\033[1m=== Alunos da Turma: {turma['nome_turma']} ===\033[m")
            if turma['alunos']:
                for aluno in turma['alunos']:
                    nome = (aluno.get('nome') or aluno.get('nome_completo') or 'N/A')
                    print(f"{aluno['numero']}. {nome}")
            else:
                print("Nenhum aluno na turma.")
            linha()
            input("Pressione Enter para continuar...")
            
        elif opcao == "6":
            return True
        else:
            print("\033[31mOpção inválida!\033[m")
            time.sleep(2)

# Função: remover_turma — confirma e remove turma selecionada
def remover_turma():
    """Remove uma turma completamente"""
    resultado = selecionar_turma_interativo()
    if not resultado:
        return False
        
    turma, index = resultado
    
    clear()
    linha()
    print(f"\033[1m=== Confirmar Remoção ===\033[m")
    print(f"Turma: {turma['nome_turma']}")
    print(f"Professor: {turma['professor']['nome']}")
    print(f"Alunos: {len(turma['alunos'])}")
    linha()
    confirmacao = input("Tem certeza que deseja remover esta turma? (s/N): ").strip().lower()
    
    if confirmacao == 's':
        turmas_data = load_turmas()
        turmas_data["turmas"].pop(index)
        save_turmas(turmas_data)
        print("\033[32mTurma removida com sucesso!\033[m")
        time.sleep(2)
        return True
    else:
        print("\033[33mRemoção cancelada.\033[m")
        time.sleep(2)
        return False

# Função: init_turmas — inicializa arquivo de turmas vazio se ausente
def init_turmas():
    if not os.path.exists(TURMAS_FILE):
        with open(TURMAS_FILE, 'w', encoding='utf-8') as f:
            json.dump({"turmas": []}, f, indent=4, ensure_ascii=False)

# Função: load_turmas — carrega turmas do arquivo JSON, inicializa se necessário
def load_turmas():
    try:
        with open(TURMAS_FILE, 'r', encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        init_turmas()
        return load_turmas()

# Função: save_turmas — salva dados de turmas no arquivo JSON
def save_turmas(turmas_data):
    with open(TURMAS_FILE, 'w', encoding='utf-8') as f:
        json.dump(turmas_data, f, indent=4, ensure_ascii=False)

# Função: criar_turma — cria uma nova turma interativamente e salva em disco
def criar_turma():
    init_turmas()
    clear()
    linha()
    print("\033[1m=== Criar Nova Turma ===\033[m")
    
    nome_turma = input("Digite o nome da turma: ").strip()
    if not nome_turma:
        print("\033[31mNome da turma é obrigatório!\033[m")
        linha()
        time.sleep(2)
        return False
    
    turmas_data = load_turmas()
    for turma in turmas_data["turmas"]:
        if turma["nome_turma"].lower() == nome_turma.lower():
            print("\033[31mJá existe uma turma com este nome!\033[m")
            linha()
            time.sleep(2)
            return False
    
    professores = load_professores()
    if not professores:
        print("\033[31mNenhum professor cadastrado no sistema!\033[m")
        print("Cadastre um professor primeiro.")
        linha()
        time.sleep(3)
        return False
    
    clear()
    linha()
    print("\033[1m=== Selecionar Professor ===\033[m")
    print("Professores disponíveis:")
    linha()
    
    for i, professor in enumerate(professores, 1):
        print(f"{i}. {professor['nome_completo'].title()} - CPF: {professor['documento']}")
    
    linha()
    try:
        escolha_prof = int(input("Selecione o professor (número): "))
        if escolha_prof < 1 or escolha_prof > len(professores):
            print("\033[31mSeleção inválida!\033[m")
            linha()
            time.sleep(2)
            return False
        
        professor_selecionado = professores[escolha_prof - 1]
    except ValueError:
        print("\033[31mDigite um número válido!\033[m")
        linha()
        time.sleep(2)
        return False
    
    alunos_data = load_matriculas()
    if not alunos_data["alunos"]:
        print("\033[31mNenhum aluno cadastrado no sistema!\033[m")
        print("Cadastre alunos primeiro através do menu de matrículas.")
        linha()
        time.sleep(3)
        return False
    
    alunos_selecionados = []
    while True:
        clear()
        linha()
        print("\033[1m=== Selecionar Alunos ===\033[m")
        print(f"Turma: {nome_turma}")
        print(f"Professor: {professor_selecionado['nome_completo'].title()}")
        print(f"Alunos selecionados: {len(alunos_selecionados)}")
        linha()
        print("Alunos disponíveis:")
        linha()
        
        alunos_disponiveis = [aluno for aluno in alunos_data["alunos"] 
                             if aluno["matricula"] not in [a["matricula"] for a in alunos_selecionados]]
        
        if not alunos_disponiveis:
            print("Todos os alunos já foram selecionados!")
            break
        
        for i, aluno in enumerate(alunos_disponiveis, 1):
            nome = (aluno.get('nome_completo') or aluno.get('nome') or 'N/A')
            print(f"{i}. {nome.title()}")
        
        linha()
        print("Digite o número do aluno para adicionar à turma")
        print("Ou digite '0' para finalizar a seleção")
        linha()
        
        try:
            escolha_aluno = int(input("Sua escolha: "))
            if escolha_aluno == 0:
                break
            elif 1 <= escolha_aluno <= len(alunos_disponiveis):
                aluno_selecionado = alunos_disponiveis[escolha_aluno - 1]
                alunos_selecionados.append(aluno_selecionado)
                print(f"\033[32m{aluno_selecionado['nome_completo'].title()} adicionado à turma!\033[m")
                time.sleep(1)
            else:
                print("\033[31mSeleção inválida!\033[m")
                time.sleep(1)
        except ValueError:
            print("\033[31mDigite um número válido!\033[m")
            time.sleep(1)
    
    if not alunos_selecionados:
        print("\033[33mNenhum aluno selecionado. Turma não criada.\033[m")
        linha()
        time.sleep(2)
        return False
    
    nova_turma = {
        "id": len(turmas_data["turmas"]) + 1,
        "nome_turma": nome_turma,
        "professor": {
            "nome": professor_selecionado["nome_completo"].lower(),
            "documento": professor_selecionado["documento"],
            "username": professor_selecionado["username"]
        },
        "alunos": []
    }
    
    for i, aluno in enumerate(alunos_selecionados, 1):
        nova_turma["alunos"].append({
            "numero": i,
            "matricula": aluno["matricula"],
            "nome": aluno["nome_completo"].lower(),
            "documento": aluno["documento"]
        })
    
    turmas_data["turmas"].append(nova_turma)
    save_turmas(turmas_data)
    
    clear()
    linha()
    print("\033[1m=== Turma Criada com Sucesso! ===\033[m")
    print(f"Nome da Turma: {nome_turma}")
    print(f"Professor: {professor_selecionado['nome_completo'].lower()}")
    print(f"Total de Alunos: {len(alunos_selecionados)}")
    linha()
    print("Alunos na turma:")
    for aluno in nova_turma["alunos"]:
        nome = (aluno.get('nome') or aluno.get('nome_completo') or 'N/A')
        print(f"{aluno['numero']}. {nome} ({aluno.get('matricula')})")
    linha()
    
    print("\033[32mTurma salva com sucesso!\033[m")
    time.sleep(3)
    return True

# Função: gerenciar_turmas — menu principal para listar, editar e remover turmas
def gerenciar_turmas():
    """Função principal de gerenciamento de turmas"""
    init_turmas()
    
    while True:
        clear()
        linha()
        print("\033[1m=== Gerenciar Turmas ===\033[m")
        print("1. Listar Todas as Turmas")
        print("2. Editar Turma")
        print("3. Remover Turma")
        print("4. Voltar")
        linha()
        
        opcao = input("Selecione uma opção (1-4): ").strip()
        
        if opcao == "1":
            clear()
            listar_turmas()
            linha()
            input("Pressione Enter para continuar...")
            
        elif opcao == "2":
            resultado = selecionar_turma_interativo()
            if resultado:
                turma, index = resultado
                editar_turma(turma, index)
                
        elif opcao == "3":
            remover_turma()
            
        elif opcao == "4":
            return True
            
        else:
            print("\033[31mOpção inválida!\033[m")
            time.sleep(2)

# Função: main — loop principal do programa com login e encaminhamento para menus
def main():
    init_users()
    
    while True:
        clear()
        linha()
        print("\033[0m Sistema de Gerenciamento de Usuários \033[m")
        print("1. Confirmar Login")
        print("2. Sair")
        linha()
        
        choice = input("Selecione uma opção (1-2): ").strip()
        
        if choice == "1":
            clear()
            linha()
            username = input("\033[1mDigite o nome de usuário: \033[m").strip()
            password = input("\033[1mDigite a senha: \033[m").strip()
            linha()
            success, role = login(username, password)
            if success:
                if role == "admin":
                    menu_admin()
                elif role == "secretaria":
                    clear()
                    if cadastrosecretaria:
                        cadastrosecretaria.menu_principal()
        elif choice == "2":
            linha()
            print("\033[31mAté logo!\033[m")
            linha()
            time.sleep(2)
            clear()
            break
        else:
            linha()
            print("\033[31mOpção inválida! Tente novamente.\033[m")
            linha()
            time.sleep(2)
            clear()

if __name__ == "__main__":
    main()
