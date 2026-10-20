# DXS: calibração isolada antes de configurar o áudio

## Resultado no Nokia G-240G-E

O teste EN751221/PEF32002 de 19/10/2026 concluiu identify, firmware,
DEV_START e BBD sem calibração. A consulta real após 3000 ms retornou
HW 0xb, FW 0x02010000 (2.1.0), duas FXS, inclusive depois dos blocos BBD.
O HOST_CFG solicitado/lido foi 0x6. O BBD tem 498 bytes e SHA-256
`e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d`.

Esses resultados validam os passos observados de controle/download;
ainda não validam calibração, áudio PCM, hook ou ringing.

## Novo modo calibrate

O DTS de identificação não declara frequência e edges PCM. O modo voice
exige essas propriedades antes de iniciar o chip; portanto tentar voice
agora falharia antes da calibração.

O novo `bringup_mode=calibrate` executa:

1. Acesso básico e download do firmware declarado no DTS.
2. DEV_START, espera `stage_wait_ms` e consulta real de versão/canais.
3. A rotina BBD/calibração já usada por voice, com as mesmas validações
   de topologia, parser e propagação de erros.
4. Consulta real imediata após a calibração, nova espera e outra consulta.
5. Retorno antes de configurar/ativar áudio PCM e registrar as linhas FXS.

Não exige parâmetros de áudio PCM, não altera o timeout da calibração,
não força sucesso e não ignora erros. Voice continua exigindo suas
propriedades PCM e a calibração normal. O novo modo faz calibração analógica
real: não é somente leitura e pode acionar circuitos internos de alimentação.
A ponte não solicita linefeed de operação nem ringing nesse modo.

O modo mantém o dispositivo inicializado até descarregar a ponte.
A remoção usa a limpeza existente, com DEV_STOP/reset. Não há nós
`/dev/en75xx-fxs*` novos enquanto o teste termina antes do registro das linhas.

## Aplicação e teste

Este patch é incremental sobre a ponte com os modos identify/firmware/start/
bbd/voice já usados nos logs. A base consultada do mirror foi o blob
`6ceb155e343c36c4a37b1344911e1f68f39dcdee` de `src/en75xx_slic_dxs.c`.
Não substitui os patches anteriores nem os arquivos de firmware.

Na raiz do airoha_voip:

```sh
git apply --check /caminho/airoha-voip-dxs-calibrate.patch
git apply /caminho/airoha-voip-dxs-calibrate.patch
```

Recompile a ponte pelo seu pacote OpenWrt e instale o módulo produzido
para o mesmo kernel da placa. Este patch não altera a ABI/exportações
do backend. O novo parâmetro só estará disponível depois de atualizar
o arquivo `en75xx-slic-dxs.ko`.

No Nokia, após atualizar o módulo:

```sh
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace
rmmod en75xx_slic_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" \
    bringup_mode=calibrate stage_wait_ms=3000
dmesg | grep -Ei 'en75xx|duslic|dxs|csi'
```

Esperado: calibração das duas portas sem erro, consultas ainda retornando
HW 0xb/FW 0x02010000/2 FXS e a mensagem `DXS calibration test passed`.
Se ocorrer falha, mantenha o log desde o início do teste, incluindo eventos
e a consulta antes da limpeza. Não prossiga para voice até entender o erro.

Depois de passar, o trabalho seguinte é conferir frequência, edges e offsets
PCM do Nokia antes de ativar voice. Não se copiam os parâmetros do MitraStar.

## Validação desta alteração

Três testes host passaram, compilando trechos C reais da ponte e do backend
com transporte/tempo simulados. Cobrem a calibração existente para um/dois
canais, o retorno antes do caminho de voz, ausência de exigência de parâmetros
PCM em calibrate, validação do BBD/topologia/espera e rejeição de erro de
calibração, retorno à ROM ou timeout nas consultas posteriores. O caminho
voice continua validando os parâmetros PCM e propagando os erros.

Patch validado com git apply e verificação de whitespace. Não foram
executados cross-build OpenWrt/MIPS ou teste deste modo em hardware.
