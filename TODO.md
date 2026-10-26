# Spet@term Roadmap & QA Tracking

Este arquivo consolida os itens para auditoria, testes contínuos e melhorias futuras da aplicação.

## QA Later

- [ ] **Item 5: Seleção de Texto e Copiar/Colar com Clique Direito no Terminal**
  - **Status:** Implementado (seleção via left-drag com cálculo de retângulo, cópia com right-click ou Ctrl+Shift+C / Ctrl+C, colagem com right-click ou Ctrl+V).
  - **Meta de QA:** Realizar testes de estresse em janelas com rolagem longa, seleções multi-bloco e verificação de integridade da área de transferência com diferentes encodings.
  - **Classificação:** QA Later

- [ ] **Item 7: Integração com Win+R (term -> Spet@term)**
  - **Status:** Registro nativo em `HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\App Paths\term.exe` implementado na aba de Configurações.
  - **Meta de QA:** Testar execução em diferentes contas de usuário Windows sem privilégios administrativos.
  - **Classificação:** QA Later

## QA MORE Later

- [ ] **Item 8: Cobertura de Sintaxe e Destaque de Comandos no Corretor**
  - **Status:** Dezenas de comandos e ferramentas de desenvolvedor (PowerShell Verb-Noun cmdlets, winget, cargo, pnpm, bun, docker, claude, gemini, ollama, cursor, git) auditados e reconhecidos com sucesso.
  - **Meta de QA:** Realizar mais baterias de testes com scripts customizados, flags raras e novos utilitários de CLI.
  - **Classificação:** QA MORE Later

- [ ] **Item 4: Transparência e Blur Acrylic do Windows 11 com DirectComposition**
  - **Status:** Implementado com `DWMWA_SYSTEMBACKDROP_TYPE` (Acrylic), cores pré-multiplicadas por canal alpha no Direct2D e preservação de opacidade 100% sólida para UI, modais e texto.
  - **Meta de QA:** Validar alternância dinâmica em tempo real (100%, 90%, 80%, 70%) em múltiplos monitores com resoluções e taxas de DPI distintas.
  - **Classificação:** QA MORE Later

- [ ] **Item 1: Execução Segura de Scripts Multilinhas (Batch / Encoded PowerShell)**
  - **Status:** Corrigido algoritmo RFC 4648 com padding em Base64 para PowerShell `-EncodedCommand`, e execução isolada via arquivo `.cmd` temporário para CMD.
  - **Meta de QA:** Testar colagem de blocos com mais de 100 linhas e caracteres especiais (`"`, `%`, `$`, `&`, `|`).
  - **Classificação:** QA MORE Later
