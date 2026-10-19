# DXS: rastrear a conclusão da calibração

Aplicar na raiz do `airoha_voip`, depois de
`airoha-voip-dxs-bbd-diagnostics.patch`.

## Resultado confirmado na placa

O log `22-29-10_1792114150.log` confirma:

- firmware 2.1.3 ativo e DEV_START concluído;
- BBD de 498 bytes chega à etapa de calibração automática;
- solicitação de calibração retorna `0x0`;
- a espera pela conclusão falha após aproximadamente 513 ms;
- `0x706d` é `DXS_statusAutomaticCalibrationTimeout`;
- `0x7055` é `DXS_statusAutomaticCalibrationFailed`, usado pelo BBD como
  erro final depois da falha anterior.

O módulo OEM MitraStar também espera 500 ms em `DXS_ALM_Calibration`.
Não há evidência para atribuir a falha a um timeout menor que o do OEM.
O BBD não foi rejeitado na integridade nem na aplicação dos blocos; falta
comprovar a conclusão e a validação da calibração.

## Caminho que será observado

O polling chama `DXS_interrupt_handler`, que lê HOST_INT1 e retira mensagens
da outbox. Um evento SDD OPC informa o modo da linha. `DXS_ALM_OnLineModeChanged`
marca a entrada em `DXS_SDD_Opmode_Calibrate` (6); quando observa a saída,
despacha `IFX_TAPI_EVENT_CALIBRATION_END_INT`. O TAPI então executa
`Calibration_Finish`, lê/verifica os resultados e acorda a espera do BBD.

Apenas o timeout não informa qual desses passos deixou de ocorrer. O patch
adiciona o parâmetro booleano `event_trace` ao backend, desligado por padrão,
para mostrar:

- primeira execução do callback de polling;
- status HOST_INT1 quando não nulo, modo de eventos e estado do dispositivo;
- header/payload inicial das mensagens de evento e erros da outbox;
- evento SDD decodificado, canal, opmode e estado de inicialização;
- opmode recebido e flag `bCalibrationRunning` antes da atualização;
- entrada e saída da rotina que finaliza/valida a calibração;
- estado em cache quando a espera expira.

Não modifica comandos, timeout, máscaras de IRQ, BBD ou alimentação da linha.
Não libera o registro da linha quando a calibração falha.

## Aplicar e testar

```sh
git apply --check /caminho/airoha-voip-dxs-calibration-event-trace.patch
git apply /caminho/airoha-voip-dxs-calibration-event-trace.patch
```

Recompile e instale `drv_tapi_dxs.ko` para o mesmo kernel da placa. O adaptador
DXS do teste anterior pode ser mantido; não há mudança de ABI nem de DTS.

No estado atual, com os dois módulos carregados após a falha de probe:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
insmod "/lib/modules/$(uname -r)/drv_tapi_dxs.ko" event_trace=1 &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 160
```

Após reiniciar, quando os dois módulos ainda não estiverem carregados, use
somente as duas linhas `insmod`, unidas por `&&`.

No OpenWrt usado no teste, os parâmetros devem ser passados por `insmod`.
Não usar `modprobe drv_tapi_dxs event_trace=1`, pois esse carregador já
demonstrou não aplicar parâmetros adicionais ao módulo.

O trace pode ser desligado sem descarregar:

```sh
echo 0 > /sys/module/drv_tapi_dxs/parameters/event_trace
```

## Interpretação

- Ausência de `background polling callback active`: investigar o timer.
- Callback ativo sem eventos SDD: investigar máscara de eventos, outbox,
  resposta do firmware e possível erro analógico reportado por ele.
- SDD OPC com opmode 6, sem saída: o host observou a entrada em calibração;
  ainda falta uma indicação de conclusão do firmware.
- Saída recebida sem entrada anterior: a flag de calibração pode não ter
  sido armada pelo evento de entrada; verificar a ordem dos eventos.
- Saída e flag armada, sem `calibration finish entered`: investigar o
  despacho diferido e a configuração do TAPI.
- `calibration finish entered` sem resultado final: investigar leitura dos
  coeficientes, locks e resposta do firmware nessa rotina.
- Finalização completa, mas espera expira: investigar sinalização do evento.

O próximo log precisa incluir as linhas anteriores ao timeout, não só os
dois códigos finais de erro.

## Validação local

Aplicação conferida nas bases correspondentes e diretivas de pré-processador
verificadas para kernel/Linux e para compilação sem kernel. Não houve build
completo do módulo para a placa nem teste físico deste trace aqui.
