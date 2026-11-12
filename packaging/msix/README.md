# Spet@Term - Empacotamento MSIX Desktop Bridge (Full Trust)

Este diretorio contem a infraestrutura completa de empacotamento MSIX para o **Spet@Term**, permitindo distribuicao nativa no Windows com instalacao em 1 clique, integracao com o Menu Iniciar e publicacao direta na **Microsoft Store**.

---

## 1. Arquitetura de Empacotamento

O Spet@Term utiliza o modelo **Desktop Bridge (Full Trust)** do Windows:
- **Execucao Nativa:** O aplicativo roda como processo Win32 puro de 64 bits (`spetaterm.exe`) com acesso irrestrito ao sistema de arquivos, ConPTY, abas com elevacao de Administrador, aceleração Direct2D/DirectWrite e decodificadores de archives.
- **Manifesto Declarativo (`AppxManifest.xml`):** Declara a capacidade restrita `<rescap:Capability Name="runFullTrust" />` e define as identidades visuais de blocos e icones.
- **Visual Assets:** Icones em resolucoes 44x44, 150x150, 310x150, 50x50 e SplashScreen gerados a partir do logotipo oficial Saturn Folder do Spet@Term.

---

## 2. Como Gerar o Pacote (.msix)

Para compilar o aplicativo (se necessario), preparar o layout e empacotar o arquivo `.msix` assinado:

```powershell
powershell -ExecutionPolicy Bypass -File .\packaging\msix\build_msix.ps1
```

O script ira:
1. Localizar automaticamente as ferramentas do Windows SDK (`makeappx.exe` e `signtool.exe`).
2. Criar a area de preparacao (`layout/`) com o manifesto, executavel e assets.
3. Gerar o pacote em `packaging\msix\dist\SpetATerm_0.0.5.0_x64.msix`.
4. Assinar digitalmente com o certificado de desenvolvimento `SpetTerm_Dev.pfx`.

---

## 3. Instalacao Local (Sideload / Testes)

Por padrao, o Windows exige que pacotes MSIX instalados fora da Store tenham um certificado confiavel na maquina local.

Para instalar o certificado de teste automaticamente (execute o PowerShell como Administrador):

```powershell
powershell -ExecutionPolicy Bypass -File .\packaging\msix\build_msix.ps1 -InstallCert
```

Apos instalar o certificado uma unica vez:
- De dois cliques em `packaging\msix\dist\SpetATerm_0.0.5.0_x64.msix` para instalar via Instalador de Aplicativos do Windows (App Installer).
- Ou instale via terminal PowerShell:
  ```powershell
  Add-AppxPackage -Path .\packaging\msix\dist\SpetATerm_0.0.5.0_x64.msix
  ```

---

## 4. Publicacao na Microsoft Store

Para submeter o Spet@Term a Microsoft Store:

1. Acesse o [Microsoft Partner Center](https://partner.microsoft.com/dashboard).
2. Va em **Windows & Xbox** > **Overview** e clique em **Create a new app**.
3. Reserve o nome do produto (ex: `Spet@Term` ou `Spet-Term`).
4. Em **Product Management** > **Product Identity**, anote:
   - **Package/Identity/Name**
   - **Package/Identity/Publisher** (ex: `CN=...`)
   - **Package/Properties/PublisherDisplayName**
5. Atualize o arquivo [AppxManifest.xml](file:///c:/Users/josem/Documents/antigravity/wonderful-salk/packaging/msix/AppxManifest.xml) com esses valores exatos fornecidos pelo Partner Center.
6. Execute o script de build novamente para gerar o `.msix` correspondente:
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\packaging\msix\build_msix.ps1
   ```
7. Na pagina de submissao do Partner Center, envie o arquivo `SpetATerm_0.0.5.0_x64.msix`.
8. A Microsoft Store realiza a validacao automatica e assina o pacote com o certificado oficial de producao da Store.
