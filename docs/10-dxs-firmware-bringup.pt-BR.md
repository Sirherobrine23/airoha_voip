# DXS: teste intermediário de firmware

Aplicar este patch sobre o port DXS anterior. Ele acrescenta o modo `firmware`
e o parâmetro `bringup_mode` ao módulo en75xx-slic-dxs; não altera o transporte
CSI nem os pinos/LEDs. O modo identify continua fazendo o mesmo teste de acesso.

O log do MitraStar confirmou o teste básico:
`DXS access test passed; no firmware/BBD or linefeed started`.
Isso confirma a comunicação de registradores CSI, mas ainda não o mailbox,
firmware, parâmetros elétricos, hook, ringing ou o caminho de áudio PCM.

O novo modo executa, nesta ordem:

1. teste básico de registradores e reset do controlador;
2. consulta da versão ROM pelo mailbox e conferência da quantidade de FXS;
3. download do firmware selecionado por `firmware-name`;
4. nova consulta de versão para verificar o firmware em execução.

Esse modo não chama DEV_START, download BBD, ativação de canal PCM, linefeed ou
ringing, nem registra /dev/en75xx-fxs*. Não exige clock-frequency ou as bordas
PCM, que só são necessários para voice. Ele altera o estado do chip, portanto
não é uma operação somente de leitura nem uma validação da parte analógica.

## Instalação e teste no MitraStar

Recompile o pacote com o novo en75xx-slic-dxs.ko para o MESMO kernel da placa
e instale-o. Não é necessário mudar/recompilar o DTS se o nó já tem:

```dts
firmware-name = "airoha-voice/DXS_FW_mitrastar.bin";
```

Confirme o arquivo:

```sh
wc -c /lib/firmware/airoha-voice/DXS_FW_mitrastar.bin
sha256sum /lib/firmware/airoha-voice/DXS_FW_mitrastar.bin
```

Esperado: 20168 bytes e SHA-256
`779afa0255e079319ccb264acba714816505c7bcda60c73b967b0c2259dae824`.

Com a ponte ainda em identify, após instalar o módulo atualizado:

```sh
modprobe -r en75xx-slic-dxs
modprobe en75xx-slic-dxs bringup_mode=firmware
dmesg | grep -Ei 'dxs|csi|firmware|failed|timeout'
```

O backend drv_tapi_dxs deve continuar carregado. Não carregue módulos .ko
compilados para outro kernel. Se a ponte estiver em voice ou em uso por Asterisk,
pare o consumidor antes de tentar descarregar o módulo.

Alternativamente, mude no DTS `airoha,bringup-mode = "firmware"` e reinicie com
o DTB atualizado. O parâmetro de módulo, quando informado, prevalece sobre DTS.

Esperamos linhas `DXS ROM: HW ... FW ..., 1 FXS`,
`DXS firmware running: HW ... FW ..., 1 FXS` e
`DXS firmware test passed; no BBD, PCM activation or linefeed started`.
O valor numérico da versão será o lido do chip; não se deve deduzi-lo apenas
do cabeçalho do arquivo PRAM.

Se falhar, envie o log completo dessas linhas e o erro de modprobe. Não mude
para voice/ring para contornar a falha: este estágio ainda é a verificação
do mailbox e da compatibilidade firmware/ROM.

## Validação deste patch

A aplicação foi conferida contra a versão anterior de en75xx_slic_dxs.c.
O teste host compila a função C real dxs_boot_firmware() com backend simulado;
confere a ordem ROM/download/versão, rejeição de quantidade de FXS incompatível
e propagação dos erros de consulta e download. Não houve build completo do
módulo para ARM/MIPS nem execução deste novo modo em hardware.
