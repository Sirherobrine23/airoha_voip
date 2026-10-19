# DXS: BOOT_FIN durante a tentativa de calibração

Aplicar depois de `airoha-voip-dxs-calibration-event-trace.patch`.

## O que o novo log comprova

O callback de polling executa e HOST_INT1 indica outbox pronta (`0x800`).
O host recebe e decodifica eventos SDD; portanto a investigação não está mais
limitada à hipótese de timer inativo.

Logo depois de `calibration start status=0x0` aparecem:

```
outbox header=0x900e000 payload0=0x0
outbox header=0x9002404 payload0=0x5
SDD channel=0 event=5 opmode=0 initialized=1
channel 1 opmode=0 calibration_running=0
```

Pelas definições do backend:

- `0x0900e000`: CMD=9, MOD=7 (SYS), ECMD=0, LENGTH=0: BOOT_FIN.
- `0x09002404`: evento SDD; payload `0x5` é OPC, opmode disabled (0).
- Não foi observada entrada em opmode calibrate (6) nessa tentativa.

Receber BOOT_FIN depois da solicitação de calibração sugere reinício do SLIC,
ou uma resposta incorreta/stale na camada de transporte. O log sozinho não
determina se houve problema de alimentação, reset externo, comando incorreto
ou falha do firmware. O status de envio zero não comprova a execução física
da calibração.

## Alterações

1. O wrapper de versão do kernel atualiza a versão do chip mesmo após
   DEV_START. A API original `DXS_Version()` usa cache nesse estado, o que
   esconderia um reinício do chip sem reinicialização do estado do host.
2. Em erro de BBD/calibração, o adaptador consulta essa versão ANTES de
   `dxs_shutdown()`. O próprio shutdown reseta o controlador, de modo que
   uma consulta depois dele não serviria para distinguir um reset inesperado.
3. O trace mostra header e payload do comando de opmode antes do envio.
4. BOOT_FIN recebe uma identificação explícita, inclusive se ocorrer após
   DEV_START. Não se altera o estado do dispositivo para aceitar a falha.
5. Eventos de erro de comando mostram motivo e header rejeitado. O trace
   também passa a incluir o segundo word do payload de evento.

Nenhum timeout ou coeficiente é alterado. O erro de BBD continua sendo
retornado mesmo se a consulta de versão funcionar. A linha não é registrada
quando a calibração falha.

## Aplicação

```sh
git apply --check /caminho/airoha-voip-dxs-unexpected-boot.patch
git apply /caminho/airoha-voip-dxs-unexpected-boot.patch
```

Recompile e instale `drv_tapi_dxs.ko` E `en75xx-slic-dxs.ko` para o mesmo
kernel da placa. Não há mudança de DTS, firmware ou BBD.

No estado atual, com os módulos ainda carregados:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
insmod "/lib/modules/$(uname -r)/drv_tapi_dxs.ko" event_trace=1 &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 200
```

Após reboot, caso ainda não estejam carregados, omitir as duas linhas rmmod.

## Ler o próximo resultado

- `opmode request=6`: confirma o valor da estrutura que chega ao envio.
  Header/payload permitem comparar a codificação com o OEM.
- `BOOT_FIN ... driver_initialized=1`: boot foi recebido depois de DEV_START.
- `DXS before cleanup ... FW 0x2000000`: uma nova consulta respondeu como ROM
  2.0.0; a execução do patch 2.1.3 foi perdida antes da limpeza do host.
- `... FW 0x2010300`: ainda responde como 2.1.3; isso não exclui um reinício
  interno com firmware retido, mas exclui a volta à ROM nessa consulta.
- Consulta de versão falha: preservar o erro e investigar a comunicação
  após BOOT_FIN. Não interpretar como identificação positiva da ROM.
- `command error`: incluir motivo e header rejeitado no diagnóstico.

## Validação local

O wrapper C real de versão foi compilado/testado em host com um backend
simulado: leitura nova após DEV_START, retorno à ROM, falha de leitura,
estado não inicializado e argumentos inválidos. Aplicação do patch conferida
nas bases correspondentes. Não houve build completo para o kernel da placa
nem validação física deste patch aqui.
