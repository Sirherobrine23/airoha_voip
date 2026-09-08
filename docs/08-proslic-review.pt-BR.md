# Revisão do port ProSLIC — 2026-09-08

Base: commit `7a966fd`, obtido do repositório do usuário. Branch de trabalho:
`codex/proslic-lifecycle`. Esta revisão preserva as correções anteriores de
PCM/ZSI e concentra as alterações no carregador de firmware e no ciclo de vida
ProSLIC. Não implementa ISI nem conclui o port de VoIP.

## Correções implementadas

| Problema encontrado | Alteração | Evidência local |
|---|---|---|
| O conversor usa `binascii.crc32`, enquanto o carregador usava o acumulador Linux sem complementos | CRC com inicialização e complemento finais compatíveis com o conversor | `tools/proslic-patch2fw.py:build_blob`, `src/en75xx_proslic_fw.c:proslic_fw_parse` |
| `n_psaddr=0` chegava ao acesso ao último elemento | Rejeição antes da alocação e da consulta ao terminador | `proslic_fw_parse` |
| Chipset, revisão e BOM do cabeçalho eram apenas impressos | Conferência contra a identidade solicitada, inclusive no arquivo de fallback | `proslic_fw_parse` |
| Reset invertia manualmente o valor lógico do GPIO | Valor lógico igual a `in_reset`; a polaridade vem do descritor GPIO | `si3219x_reset`, API GPIO do kernel disponível |
| Trocar `bom` sugeria suporte a outras topologias elétricas | Aceita somente `lcqc`, correspondente ao patch usado com a configuração LCCB compilada | `src/Makefile`, `vendor/proslic/custom/si3219x_LCCB_constants.c` |
| Inicialização alterava símbolos globais de patch com buffers por dispositivo | Mutex durante publicação/consumo; limpeza dos símbolos antes de liberar o mutex | `en75xx_si3219x_api_init`, seleção de patch em `vendor/proslic/src/si3219x_intf.c` |
| Canal PCM inválido era detectado após energizar o conversor | Validação do bit offset antes de inicializar a API | `en75xx_si3219x_api_init` |
| Falha de LBCal era somente um aviso | Interrompe a inicialização e executa a limpeza | `ProSLIC_LBCal`, `en75xx_si3219x_api_init` |
| Erros após início da API e remoção podiam deixar o conversor ativo | Tenta parar PCM e desligar o conversor, solicita linha aberta e afirma reset antes de liberar recursos | `ProSLIC_PowerDownConverter` em `vendor/proslic/src/proslic.c` |
| Falha ao registrar a linha vazava o firmware | Limpeza comum de API e firmware | `en75xx_si3219x_probe` |
| Reinício/desligamento não tinham callback do SLIC | Callback `shutdown` cancela os trabalhos e tenta desligar o conversor | `en75xx_si3219x_shutdown` |
| Exemplos habilitavam Si32192 sob SPI, apesar da matriz de suporte indicar ISI | Nós Si32192 desabilitados; probe recusa compatibles Si32192/Si32193 antes das operações de reset e SPI desse driver | `docs/07-gpl-sdk-crosscheck.md`, manual fornecido `Airoha_Doc/VOIP/VOIP Reference Module En.pdf`, página 188 |

A configuração LCCB não é validada para a sua placa apenas porque o nome do
patch é LCQC. Perfil elétrico, revisão do chip e circuito externo continuam
precisando corresponder. O compatible genérico `silabs,si3219x` permanece como
protótipo; renomear um Si32192 para esse compatible não implementa ISI.

O desligamento é uma tentativa pelo software, não uma garantia de ausência de
tensão. A própria rotina do fornecedor transita por FWD_OHT antes de OPEN e
aguarda a descarga de VBAT. Erros de transporte ou fontes externas podem impedir
o resultado. Não há medição física nesta revisão. O bloqueio do probe também
não controla ações realizadas anteriormente pelos drivers pais ou pelo bootloader.

## PCM, ZSI e IRQs

As correções de timeslots, deslocamento TX ZSI e comprimentos MPI já estavam no
commit base; não foram reimplementadas nesta revisão. A ausência de `0xa8` na
tabela exportada foi reclassificada na documentação: ela não prova que o
registrador inexista. Não se deve escrever nesse offset sem identificar sua função.

| Família | PCM1 / PCM | PCM2 | Outro periférico | Proveniência |
|---|---:|---:|---|---|
| EN751221 | 11 | 33 | — | Informação fornecida pelo usuário |
| EN751627 / EN7528 | 11 | 32, a confirmar como PCM2 | — | O usuário repetiu o rótulo PCM1 no segundo item |
| EN7523 | 27 | Não estabelecido aqui | I2S: 48 | Usuário; PCM SPI 27 também registrado no cross-check do DTS do SDK |

Não confundir esses números de interrupção de hardware com o IRQ virtual
alocado pelo Linux. I2S 48 não é PCM2. O mapeamento MIPS para o controlador de
interrupções ainda requer conferência no DTS/irqchip do kernel de destino.

## Validação realizada

- `python3 tests/test_proslic_fw.py -v`: seis testes passaram. Compila o código C
  real do carregador com adaptações para o host e UBSan; aceita um firmware
  produzido pelo conversor e rejeita identidade divergente, contagens nulas,
  truncamento, corrupção de CRC e ausência de terminadores.
- Objetos dos cinco módulos compilados com a árvore local Linux
  `6.18.41-g8f909dac6c5a`, configuração x86-64. Inclui compilação e link relocável
  do código ProSLIC e fornecedor. Isso não é uma compilação cruzada MIPS/ARM.
- O alvo completo `modules` parou em MODPOST porque falta o `Module.symvers` do
  kernel. Não foi contornado esse erro nem produzido um conjunto de módulos
  validado para carregar na placa.
- `git diff --check` sem erros.
- Nenhum módulo carregado, MMIO executado ou teste elétrico realizado.

## Pendências para continuar

1. Implementar o transporte ISI a partir dos símbolos/relocações e fluxo de
   inicialização do BSP, preservando as diferenças entre famílias. Não assumir
   que o protocolo e a configuração de ZSI servem diretamente para ISI.
2. Separar diagnóstico digital de inicialização de alimentação/ringing. O
   caminho genérico ainda executa `ProSLIC_Init`, calibração e alimentação de
   linha; não é uma sonda de identificação passiva.
3. Tratar erros de transporte por toda a API, inclusive operações RAM compostas,
   fixups e trabalhos de ringing. Vários retornos ainda são ignorados.
4. Rever a vida útil da interface `/dev/en75xx-fxsN` durante unbind com arquivos
   abertos, e serializar operações completas do SLIC. O mutex desta revisão
   protege símbolos de firmware durante Init, não todas essas operações.
5. Validar perfil/BOM, clocks, polaridade/compartilhamento de reset e temporização
   na placa específica antes de habilitar o FXS. Continuam pendentes testes de
   DMA/IRQ/áudio, proteções e desligamento físico.

Aplicação do patch entregue: numa árvore limpa em `7a966fd`, executar
`git apply --check driver-changes.patch` e depois `git apply driver-changes.patch`.
