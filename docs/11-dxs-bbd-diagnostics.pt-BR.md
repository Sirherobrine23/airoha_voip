# Diagnóstico do erro após carregar o firmware DXS

Aplicar depois de `mitrastar-dxs-pcm-timing.tar.gz`, na raiz do `airoha_voip`.

O log `22-09-27_1792112967(1).log` mostra o backend carregado,
parâmetros 2.048 MHz, bordas TX/RX 0/1 e offsets 7/1, e firmware 2.1.3
em execução. Depois disso há `-EIO`, sem a mensagem `DXS BBD accepted`.
Esse log ainda não comprova o registro da linha ou o funcionamento do áudio.

O intervalo entre a consulta do firmware (213.292367) e o erro (213.851524)
é aproximadamente 559 ms. O backend define `DXS_SDD_CALIBRATE_TMOUT_MS = 500`
e realiza calibração automática depois do BBD. Isso é compatível com timeout
de calibração, mas NÃO basta para concluir a causa. O código atual converte
o status do backend em `-EIO`, ocultando a informação necessária.

## O que o patch adiciona

- Mensagem após DEV_START, antes de iniciar o BBD.
- Resultado da integridade do BBD quando falha.
- Tag, índice e status do bloco BBD que falhou.
- Status original da calibração antes de ser convertido em erro genérico.
- Resultado do início e da espera de calibração.
- Status final DXS e topologia permitida pelo backend.

As mensagens não dependem da configuração opcional de trace do fornecedor.
O patch só acrescenta diagnóstico. Não altera timeout, clock, offsets,
perfil DC/DC, sequência de inicialização nem critérios de sucesso.

## Aplicação e instalação

```sh
git apply --check /caminho/airoha-voip-dxs-bbd-diagnostics.patch
git apply /caminho/airoha-voip-dxs-bbd-diagnostics.patch
```

Recompile e instale AMBOS os módulos para o mesmo kernel da placa:

- `en75xx-slic-dxs.ko`
- `drv_tapi_dxs.ko`

Não há alteração de DTS; o DTB do teste atual pode continuar sendo usado.
O código de BBD e calibração pertence a `drv_tapi_dxs.ko`: substituir só o
adaptador não produz os logs internos.

Com ambos os módulos carregados no estado de falha mostrado pelo log:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
modprobe drv_tapi_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 120
```

Depois de reiniciar a placa, quando esses dois módulos ainda não estiverem
carregados, use apenas:

```sh
modprobe drv_tapi_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 120
```

Interpretação:

- Falha em DEV_START: o adaptador já informa o número do comando TAPI.
- `integrity check failed`: o parser rejeitou o formato/integridade.
- `block tag=... status=...`: falha durante aplicação daquele bloco.
- `calibration start status=...`: resultado da solicitação da calibração.
- `calibration wait failed`: erro na espera do evento de conclusão.
- `automatic calibration status=...`: status antes da conversão em
  `DXS_statusAutomaticCalibrationFailed`.
- `BBD failed: DXS status=...`: status original da API, antes de virar `-EIO`.

Não confundir o módulo listado em `lsmod` com sucesso do probe do dispositivo:
um módulo pode permanecer carregado mesmo quando `dxs-fxs` falha no probe.

## Validação

Aplicação dos patches verificada nas bases locais correspondentes. Os testes
em host de firmware e parâmetros PCM continuam passando. Este patch de logs
não foi compilado para o kernel da placa nem validado fisicamente aqui.
