import sys
import json
import os
import time
import shutil
import random

sys.stdout.reconfigure(encoding='utf-8')

# Determina o diretório base: se estiver empacotado (frozen) usa o diretório do executável,
# caso contrário usa o diretório do script atual.
if getattr(sys, 'frozen', False):
    BASE_DIR = os.path.dirname(sys.executable)
else:
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Define PYTHON_EMBEDDED_DIR apontando para a pasta 'python_embedded' se já estivermos nela,
# caso contrário cria o caminho relativo dentro do BASE_DIR.
current_dir_name = os.path.basename(BASE_DIR)
if current_dir_name == 'python_embedded':
    PYTHON_EMBEDDED_DIR = BASE_DIR
else:
    PYTHON_EMBEDDED_DIR = os.path.join(BASE_DIR, "python_embedded")

# Garante que a pasta python_embedded exista
os.makedirs(PYTHON_EMBEDDED_DIR, exist_ok=True)

# Caminho absoluto do arquivo JSON que armazena as matrículas
MATRICULA_FILE = os.path.join(PYTHON_EMBEDDED_DIR, "matriculas.json")

# Formata um CPF no padrão ###.###.###-## se tiver 11 dígitos; caso contrário retorna o valor original.
def formatar_cpf(cpf):
    if len(cpf) == 11 and cpf.isdigit():
        return f"{cpf[:3]}.{cpf[3:6]}.{cpf[6:9]}-{cpf[9:]}"
    return cpf

# Remove todos os caracteres não numéricos (usado para comparar CPFs)
def desformatar_cpf(cpf):
    return ''.join(filter(str.isdigit, cpf))

# Formata data DDMMAAAA para DD/MM/AAAA se tiver 8 dígitos; caso contrário retorna o valor original.
def formatar_data_nascimento(data):
    if len(data) == 8 and data.isdigit():
        return f"{data[:2]}/{data[2:4]}/{data[4:]}"
    return data

# Formata telefone brasileiro com 11 dígitos para (XX) XXXXX-XXXX; caso contrário retorna o valor original.
def formatar_telefone(telefone):
    if len(telefone) == 11 and telefone.isdigit():
        return f"({telefone[:2]}) {telefone[2:7]}-{telefone[7:]}"
    return telefone

# Retorna a largura atual do terminal; se não for possível, usa 80 como padrão.
def checar_largura():
    try:
        return shutil.get_terminal_size().columns
    except (AttributeError, OSError):
        return 80

# Imprime uma linha horizontal usando o símbolo fornecido repetido pela largura do terminal.
def linha(simbolo='='):
    largura = checar_largura()
    print(simbolo * largura)

# Limpa a tela conforme o sistema operacional.
def clear():
    os.system("cls" if os.name == "nt" else "clear")

# Pausa a execução por 2 segundos.
def sleep():
    time.sleep(2)

# Limpa a tela, mostra mensagem de saída e encerra o programa.
def sair():
    clear()
    linha()
    print("\033[31mSaindo....\033[m")
    linha()
    sys.exit()

# Carrega o dicionário de matrículas do arquivo JSON; cria o arquivo vazio se não existir.
def carregar_matricula():
    try:
        with open(MATRICULA_FILE, "r", encoding='utf-8') as f:
            return json.load(f)
    except FileNotFoundError:
        with open(MATRICULA_FILE, "w", encoding='utf-8') as f:
            json.dump({}, f, ensure_ascii=False)
        return {}

# Interage com o usuário para criar uma nova matrícula, valida entradas, garante CPF único e salva no JSON.
def criar_matricula():
    linha()
    print("=== Criar Matrícula ===")
    nomealuno = input("Digite o nome completo do aluno: ").lower().strip()
    docaluno = input("Digite o CPF do aluno (Somente Números): ").strip()
    datanascimento = input("Digite a data de nascimento (DDMMAAAA): ").strip()
    telefone = input("Digite o telefone do aluno (Somente Números, 11 dígitos): ").strip()
    endereco = input("Digite o endereço do aluno: ").lower().strip()
    sexo = input("Digite o sexo do aluno (M/F/Outro): ").lower().strip()
    
    # Validações
    if not all([nomealuno, docaluno, datanascimento, telefone, endereco, sexo]):
        print("\033[31mTodos os campos são obrigatórios!\033[m")
        sleep()
        return False, None
    
    if len(docaluno) != 11 or not docaluno.isdigit():
        print("\033[31mCPF inválido! Deve conter 11 dígitos numéricos.\033[m")
        sleep()
        return False, None
    
    if len(datanascimento) != 8 or not datanascimento.isdigit():
        print("\033[31mData de nascimento inválida! Use o formato DDMMAAAA.\033[m")
        sleep()
        return False, None
    
    if len(telefone) != 11 or not telefone.isdigit():
        print("\033[31mTelefone inválido! Deve conter 11 dígitos numéricos.\033[m")
        sleep()
        return False, None
    
    if sexo not in ["m", "f", "outro"]:
        print("\033[31mSexo inválido! Use M, F ou Outro.\033[m")
        sleep()
        return False, None
    
    matricula = carregar_matricula()
    
    # Verifica se o CPF já existe
    for matricula_id, dados in matricula.items():
        if desformatar_cpf(dados["documento"]) == docaluno:
            print("\033[31mCPF já cadastrado!\033[m")
            sleep()
            return False, None
    
    # Gera matrícula única
    while True:
        matriculaaluno = ''.join(random.choices('0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ', k=6))
        if matriculaaluno not in matricula:
            break
    
    docaluno_formatado = formatar_cpf(docaluno)
    datanascimento_formatada = formatar_data_nascimento(datanascimento)
    telefone_formatado = formatar_telefone(telefone)
    
    print("\033[32mMatrícula criada com sucesso!\033[m")
    linha()
    nomealuno_exib = nomealuno.title()
    print(f"A matrícula gerada do aluno {nomealuno_exib} é: \033[32m{matriculaaluno}\033[m")
    matricula[matriculaaluno] = {
        "nome": nomealuno,
        "data_nascimento": datanascimento_formatada,
        "documento": docaluno_formatado,
        "telefone": telefone_formatado,
        "endereco": endereco,
        "sexo": sexo
    }
    with open(MATRICULA_FILE, "w", encoding='utf-8') as f:
        json.dump(matricula, f, indent=4, ensure_ascii=False)
    input("Pressione Enter para retornar ao menu...")
    clear()
    return True, matriculaaluno

# Busca um aluno por nome ou CPF, permite editar os campos com validação e salva as alterações no JSON.
def editar_matricula():
    linha()
    print("=== Editar Matrícula ===")
    print("Buscar aluno por:")
    print("1 - Nome")
    print("2 - CPF")
    escolha = input("Escolha uma opção: ").strip()
    
    matricula = carregar_matricula()
    matricula_num = None
    if escolha == "1":
        linha()
        nome_busca = input("Digite o nome completo do aluno: ").lower().strip()
        for matricula_id, dados in matricula.items():
            if dados["nome"] == nome_busca:
                matricula_num = matricula_id
                break
        if not matricula_num:
            linha()
            print("\033[31mNome não encontrado.\033[m")
            linha()
            sleep()
            return False
    elif escolha == "2":
        linha()
        documento_busca = input("Digite o CPF do aluno (Somente Números): ").strip()
        documento_desformatado = desformatar_cpf(documento_busca)
        for matricula_id, dados in matricula.items():
            if desformatar_cpf(dados["documento"]) == documento_desformatado:
                matricula_num = matricula_id
                break
        if not matricula_num:
            linha()
            print("\033[31mCPF não encontrado.\033[m")
            linha()
            sleep()
            return False
    else:
        linha()
        print("\033[31mOpção inválida.\033[m")
        linha()
        sleep()
        return False
    
    # Exibe os dados atuais
    dados = matricula[matricula_num]
    linha()
    print("Dados atuais do aluno:")
    print(f"Nome: {dados['nome'].title()}")
    print(f"Documento: {dados['documento']}")
    print(f"Data de Nascimento: {dados['data_nascimento']}")
    print(f"Telefone: {dados['telefone']}")
    print(f"Endereço: {dados['endereco'].title()}")
    print(f"Sexo: {dados['sexo'].title()}")
    linha()
    
    # Solicita novos dados
    print("Digite os novos dados (deixe em branco para manter o atual):")
    novo_nome = input(f"Novo nome [{dados['nome'].title()}]: ").lower().strip() or dados["nome"]
    novo_doc = input(f"Novo CPF (Somente Números) [{dados['documento']}]: ").strip() or desformatar_cpf(dados["documento"])
    nova_data = input(f"Nova data de nascimento (DDMMAAAA) [{dados['data_nascimento']}]: ").strip() or dados["data_nascimento"].replace("/", "")
    novo_telefone = input(f"Novo telefone (Somente Números, 11 dígitos) [{dados['telefone']}]: ").strip() or desformatar_cpf(dados["telefone"])
    novo_endereco = input(f"Novo endereço [{dados['endereco'].title()}]: ").lower().strip() or dados["endereco"]
    novo_sexo = input(f"Novo sexo (M/F/Outro) [{dados['sexo'].title()}]: ").lower().strip() or dados["sexo"]
    
    # Validações
    if len(novo_doc) != 11 or not novo_doc.isdigit():
        print("\033[31mCPF inválido! Deve conter 11 dígitos numéricos.\033[m")
        sleep()
        return False
    
    if len(nova_data) != 8 or not nova_data.isdigit():
        print("\033[31mData de nascimento inválida! Use o formato DDMMAAAA.\033[m")
        sleep()
        return False
    
    if len(novo_telefone) != 11 or not novo_telefone.isdigit():
        print("\033[31mTelefone inválido! Deve conter 11 dígitos numéricos.\033[m")
        sleep()
        return False
    
    if novo_sexo not in ["m", "f", "outro"]:
        print("\033[31mSexo inválido! Use M, F ou Outro.\033[m")
        sleep()
        return False
    
    # Formata os novos dados
    novo_doc_formatado = formatar_cpf(novo_doc)
    nova_data_formatada = formatar_data_nascimento(nova_data)
    novo_telefone_formatado = formatar_telefone(novo_telefone)
    
    # Atualiza os dados no dicionário
    matricula[matricula_num] = {
        "nome": novo_nome,
        "data_nascimento": nova_data_formatada,
        "documento": novo_doc_formatado,
        "telefone": novo_telefone_formatado,
        "endereco": novo_endereco,
        "sexo": novo_sexo
    }
    
    # Salva no arquivo JSON
    with open(MATRICULA_FILE, "w", encoding='utf-8') as f:
        json.dump(matricula, f, indent=4, ensure_ascii=False)
    
    print("\033[32mDados atualizados com sucesso!\033[m")
    linha()
    input("Pressione Enter para retornar ao menu...")
    clear()
    return True

# Busca e exibe dados do aluno pela matrícula fornecida; retorna (sucesso, dados).
def buscar_aluno_por_matricula():
    matricula = carregar_matricula()
    linha()
    print("=== Buscar Aluno por Matrícula ===")
    matriculaaluno = input("Digite a matrícula do aluno que deseja pesquisar: ").strip()
    if matriculaaluno in matricula:
        linha()
        print(f"Nome: {matricula[matriculaaluno]['nome'].title()}")
        print(f"Documento: {matricula[matriculaaluno]['documento']}")
        print(f"Data de Nascimento: {matricula[matriculaaluno]['data_nascimento']}")
        print(f"Telefone: {matricula[matriculaaluno]['telefone']}")
        print(f"Endereço: {matricula[matriculaaluno]['endereco'].title()}")
        print(f"Sexo: {matricula[matriculaaluno]['sexo'].title()}")
        linha()
        input("Pressione Enter para retornar ao menu...")
        clear()
        return True, matricula[matriculaaluno]
    else:
        linha()
        print("\033[31mMatrícula não encontrada.\033[m")
        linha()
        input("Pressione Enter para retornar ao menu...")
        clear()
        return False, None

# Busca e exibe dados do aluno pelo CPF; retorna (sucesso, dados, matrícula).
def buscar_aluno_por_documento():
    matricula = carregar_matricula()
    linha()
    print("=== Buscar Aluno por Documento ===")
    documentoaluno = input("Digite o documento do aluno que deseja pesquisar (Somente Números): ").strip()
    documentoaluno_desformatado = desformatar_cpf(documentoaluno)
    for matricula_num, dados in matricula.items():
        if desformatar_cpf(dados.get("documento")) == documentoaluno_desformatado:
            linha()
            print(f"Nome: {dados['nome'].title()}")
            print(f"Matrícula: {matricula_num}")
            print(f"Documento: {dados['documento']}")
            print(f"Data de Nascimento: {dados['data_nascimento']}")
            print(f"Telefone: {dados['telefone']}")
            print(f"Endereço: {dados['endereco'].title()}")
            print(f"Sexo: {dados['sexo'].title()}")
            linha()
            input("Pressione Enter para retornar ao menu...")
            clear()
            return True, dados, matricula_num
    linha()
    print("\033[31mDocumento não encontrado.\033[m")
    linha()
    input("Pressione Enter para retornar ao menu...")
    clear()
    return False, None, None

# Mostra o menu principal e direciona para as operações correspondentes.
def menu_principal():
    while True:
        linha()
        print("=== Sistema de Matrículas ===")
        print("1 - Criar Matrícula")
        print("2 - Pesquisar Aluno por Matrícula")
        print("3 - Pesquisar Aluno por Documento")
        print("4 - Editar Dados do Aluno")
        print("5 - Sair")
        linha()
        escolha = input("Escolha uma opção: ").strip()
        if escolha == "1":
            clear()
            criar_matricula()
        elif escolha == "2":
            clear()
            buscar_aluno_por_matricula()
        elif escolha == "3":
            clear()
            buscar_aluno_por_documento()
        elif escolha == "4":
            clear()
            editar_matricula()
        elif escolha == "5":
            sair()
        else:
            clear()
            linha()
            print("\033[31mOpção inválida. Tente novamente em 2 segundos...\033[m")
            sleep()