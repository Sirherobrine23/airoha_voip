# MitraStar EN7523: comparação dos módulos OEM com o port DXS

## Escopo e limites

Foram lidos os três exports Ghidra enviados: `slic3_intel.ko.c`,
`slic3_intel_latam_25.ko.c` e `slic3_intel_latam_25_BR.ko.c`.
Eles identificam o driver OEM como `drv_dxs-1.3.1.0`; o backend do port é
o pacote standalone `drv_tapi_dxs-1.0.0.0`, que inclui DXS e TAPI.
Esses números nomeiam pacotes diferentes e não estabelecem, sozinhos,
equivalência ou ordem de atualização entre as implementações.

As decompilações permitem comparar fluxo, constantes e chamadas. Não são
fontes prontas para recompilar: há símbolos sem tipo, retornos inferidos como
void, globais sem conteúdo e variáveis `extraout_*`/`unaff_*`. Não se deve
copiar esses artefatos literalmente. Arrays apontados por DAT/UNK/LANCHOR
precisam dos ELFs ou de extrações verificadas para comparação dos bytes.

## Diferenças entre os três exports

O parser local encontrou 556 funções nos dois primeiros arquivos e 557 no BR.
Entre `slic3_intel` e `latam_25`, somente `dxs_proc_VersionGet` tem corpo
textualmente diferente: data/hora de compilação, Jun 6 2023 16:18:02 contra
Dec 12 2022 14:05:13, ambos kernel 4.4.115. Isso descreve os exports de C;
não é uma afirmação de igualdade dos arquivos ELF ou de suas seções de dados.

Entre os três há 517 funções nomeadas em comum, das quais 422 têm corpos
textualmente idênticos. Parte das diferenças restantes é deslocamento de
endereços/labels ou alteração de nomes temporários do Ghidra.
A variante BR inclui dumps de BBD, comandos mailbox e eventos, além de
diferenças pontuais em outras rotinas. Não foi estabelecida equivalência
semântica de todas as funções nem dos perfis regionais de dados.

## Caminho crítico comparado

Referências abaixo usam linhas de `slic3_intel.ko.c` enviado nesta conversa.

| Função OEM | Linha | Comparação com o port |
|---|---:|---|
| IntelSlicInit | 21850 | Acesso básico, leitura ROM, download, DEV_START, BBD/calibração, configuração PCM e linefeed. A ordem geral corresponde ao port. |
| DXS_BBD_Download | 18773 | Valida BBD/topologia e inicia calibração nos canais marcados como necessários. O erro 0x7055 encapsula falha da calibração; não identifica a causa física. |
| DXS_ALM_Calibration | 25080 | Inicia em modo interno e espera evento por 500 ms, como no backend atual. |
| DXS_ALM_OpmodeSet | 24794 | Marca mudança pendente e envia a estrutura de opmode pela mailbox. |
| DXS_ALM_InitCh | 24659 | Inicializa header CMD=1, ECMD=7, canal zero-based e payload de opmode. |
| DXS_ALM_OnLineModeChanged | 24629 | Observar modo 6 marca calibração em andamento; sair desse modo dispara o evento interno de conclusão. |
| DXS_TAPI_LL_ALM_Calibration_Finish | 23879 | Lê/verifica resultados e acorda o chamador. Não é correto substituí-la por sucesso artificial no timeout. |
| dxs_polling_mode_timer | 17639 | Percorre dispositivos em polling com acesso básico inicializado. |
| DXS_PollingModeTimerStart | 17692 | Período de 10 ms; o OEM não exige IRQ externo nessa fase. |
| DXS_interrupt_handler | 17574 | Lê INT1/INT2, trata erros e outbox quando INT1 & 0x800. |
| DXS_CmdRead | 22767 | Envia leitura, espera resposta pela outbox e entrega words via FIFO do host. A leitura de versão depende dessa cadeia. |
| DXS_ResetController | 16061 | Desabilita canais 0 e 1 e depois escreve HOST_CFG=0x84. O envio ao canal 1 durante reset também existe no OEM. |
| DXS_FW_Start | 16234 | Diferença concreta: ao final, o OEM escreve HOST_CFG=7 para CSI e 3 para SPI convencional. |

O comando de calibração do canal zero reconstruído do OEM corresponde a
header `0x01000704`, payload `0x00060000`, como no trace atual. Em CSI, os
dois words seguem em fragmentos com prefixo `7e 14`:

```text
7e 14 01 00 07 04
7e 14 00 06 00 00
```

Esses bytes são a codificação esperada a partir das estruturas e do acesso
OEM, não uma captura física do barramento da placa testada.

## Diferença de HOST_CFG corrigida por este patch

Os três exports mostram em `DXS_FW_Start`:

```c
if (slicInterfType == 1)
    value = 7;
else
    value = 3;
DXS_RegWrite(device, 6, value);
```

O ELF OEM previamente disponível nesta sessão possui a mesma função
decompilada. A checagem das instruções ARM confirma `MOVEQ r2,#7` em
`.text+0x1488c`, `MOVNE r2,#3` em `.text+0x14890` e relocation R_ARM_CALL
para DXS_RegWrite em `.text+0x14894`.

O port escrevia `SC_MD_SCON | 8BIT_EN`, isto é, 0x06, também em CSI.
As definições de `drv_dxs.h` explicam a diferença:

| Bit de HOST_CFG | Valor | Significado no header do driver |
|---|---:|---|
| INT_MD_INTPOS | 0x01 | Polaridade da saída de interrupção |
| SC_MD_SCON | 0x02 | Modo de limpeza automática dos indicadores de interrupção |
| 8BIT_EN | 0x04 | Acesso SPI de 8 bits |
| RST_RSTCORE | 0x80 | Reset do core |

O patch faz o CSI EN7523 escrever 0x07 depois do download e ler de volta os bits
0..2. Erro de leitura ou divergência impede prosseguir. O teste inicial de
acesso continua usando sua própria configuração. Os demais controladores
preservam 0x06, sem aplicar indiscriminadamente o 0x03 do OEM, cuja configuração
de largura de acesso difere da do backend atual.

Isso alinha um comportamento concreto ao OEM. Como o port usa polling e
não depende do pino IRQ externo, a diferença de polaridade NÃO comprova a
causa do reset/travamento durante calibração. O patch não altera potência,
coeficientes, timeout, comando opmode, clocks ou critérios de calibração.
HOST_CFG é registro interno do DXS acessado por CSI/SPI, não um endereço
MMIO do EN7523 para escrita com devmem.

## O que os logs já localizam

No teste atual, DEV_START e BBD sem calibração preservam o firmware 2.1.1
após esperas de 3 s, confirmadas por novas consultas. No teste voice:
480.060961 s ainda há 2.1.1; em 480.071064 s é enviado opmode=6;
a espera expira com pending=1 e depois a consulta de versão não responde.
Nessa tentativa não houve BOOT_FIN posterior nem identificação positiva de ROM.
Tentativas anteriores tiveram BOOT_FIN e leitura de ROM 2.0.0 antes da limpeza.
São duas manifestações que devem continuar separadas no diagnóstico.

O erro `rejected_header=0x01010704` corresponde ao canal 1 durante reset.
A própria função OEM envia aos dois canais. Portanto isso não demonstra
erro exclusivo do port nem explica sozinho a falha posterior.

## Aplicar e testar

Aplicar na raiz da árvore que já tem os patches CSI e stage-isolation:

```sh
git apply --check /caminho/airoha-voip-dxs-oem-hostcfg.patch
git apply /caminho/airoha-voip-dxs-oem-hostcfg.patch
```

Recompilar e instalar `drv_tapi_dxs.ko` para o mesmo kernel do dispositivo.
O adaptador não precisa de API nova neste patch. Manter firmware/BBD/timing
do teste anterior para mudar apenas a configuração aqui identificada.

Com ambos os módulos carregados, depois de substituir o backend em disco:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
modprobe drv_tapi_dxs &&
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 160
```

Esperado antes da calibração:
`runtime HOST_CFG: requested=0x7 read=0x7 status=0x0`.
Se falhar novamente, guardar os eventos e a consulta antes da limpeza.
Não omitir a calibração nem registrar a linha como funcional após timeout.

O teste de host `tests/test_dxs_runtime_hostcfg.py` compila as funções C reais
de seleção/programação com SPI simulado. Cobre a alteração CSI EN7523 e a
preservação de EN751221 e outros
compatibles, ausência de nó/dispositivo, falhas de escrita/leitura e readback
incompatível. Passou com `cc -Wall -Werror`. Build completo OpenWrt e execução
na placa ainda são necessários.

Se o problema persistir, os próximos dados necessários são a continuidade
do polling e as respostas de registradores HOST não destrutivos após opmode=6;
em paralelo, uma captura física de CSI/reset/alimentação pode separar falha
de protocolo, clock e queda de alimentação. Esses fenômenos não podem ser
determinados somente pela decompilação ou pelo sucesso do envio do comando.
