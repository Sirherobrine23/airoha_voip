# DXS: separar DEV_START, BBD e calibração

Patch incremental para a árvore com `airoha-voip-dxs-unexpected-boot.patch`
e os patches de diagnóstico/timing anteriores. Não muda DTS, firmware,
coeficientes, comando de calibração nem o timeout de 500 ms do backend.

## Resultado dos dois firmwares

No MitraStar/PEF32001, tanto o firmware OEM 2.1.3 como a imagem alternativa,
que o chip identifica como 2.1.1, voltam à ROM 2.0.0 durante a sequência
BBD/calibração. Nos dois casos, BOOT_FIN aparece cerca de 36 ms após o
registro do comando opmode=6. A consulta antes da limpeza confirma ROM.
Isso reduz a hipótese de um problema exclusivo da imagem 2.1.3, sem
determinar se a causa é software, transporte, clocks, alimentação ou reset.

## Novos pontos de parada

| bringup_mode | Executa | Não executa |
|---|---|---|
| identify | Acesso básico/reset | Download, DEV_START, BBD e voz |
| firmware | Download, espera e leitura real da versão | DEV_START, BBD e voz |
| start | Download, DEV_START, espera e leitura real da versão | BBD e voz |
| bbd | Download, DEV_START, BBD, esperas e leituras reais da versão | Calibração automática e voz |
| voice | Sequência normal, com leituras adicionais da versão | Não omite calibração |

Os modos de diagnóstico terminam antes da configuração/ativação PCM do
DXS, do linefeed e do registro da linha FXS. O controlador PCM do SoC pode
continuar ligado pelo seu próprio driver. Aplicar BBD configura o circuito
analógico; o modo `bbd` não representa garantia de ausência de risco físico.
Usar apenas o BBD já identificado para a placa e sua topologia CIBB12.

`stage_wait_ms` controla a espera dos diagnósticos, padrão 1000 ms,
intervalo 100..60000 ms. O insmod aguarda essas pausas. No modo `bbd`, há
uma pausa após DEV_START e outra após o BBD. No modo `voice`, esse parâmetro
não introduz pausas. A leitura da versão usa o wrapper atualizado, que
consulta o chip mesmo depois de DEV_START.

O backend mantém `bCalibrationNeeded` pendente no modo `bbd`; esse estado
não deve ser usado para chamadas. Somente a entrada de diagnóstico deixa
de iniciar a calibração. O ioctl original e o caminho `voice` continuam
exigindo-a. Reiniciar o adaptador em `voice` executa novamente todo o bring-up.

Antes da calibração, o backend consulta a versão após todos os blocos do BBD.
Se a versão mudar ou a consulta falhar, não inicia calibração. O adaptador
também verifica a identidade do firmware após DEV_START e após calibração,
antes de configurar PCM/registrar FXS. Uma mudança retorna erro; não é
interpretada como teste aprovado.

## Aplicar e compilar

Na raiz de airoha_voip:

```sh
git apply --check /caminho/airoha-voip-dxs-stage-isolation.patch
git apply /caminho/airoha-voip-dxs-stage-isolation.patch
```

Recompile e instale AMBOS `drv_tapi_dxs.ko` e `en75xx-slic-dxs.ko` para
o kernel em execução. O adaptador passa a depender do novo símbolo
`en75xx_dxs_kernel_bbd_prepare`; misturar backend antigo e adaptador novo
causa Unknown symbol. O patch não inclui binários compilados.

## Teste 1: DEV_START sem BBD

Com ambos os módulos carregados, após substituir os arquivos em disco:

```sh
rmmod en75xx_slic_dxs &&
rmmod drv_tapi_dxs &&
modprobe drv_tapi_dxs &&
echo 1 > /sys/module/drv_tapi_dxs/parameters/event_trace &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=start stage_wait_ms=3000

dmesg | tail -n 100
```

Após reboot, se não estiverem carregados, omitir os dois rmmod. O modprobe
carrega o backend/dependências; insmod do adaptador precisa desse backend.
Um probe que falhou ainda pode deixar o módulo carregado: verificar o log,
não somente lsmod. Este teste não cria `/dev/en75xx-fxs0`.

Esperado: `after DEV_START` com a MESMA versão anunciada por
`DXS firmware running`, seguido de `DXS DEV_START test passed`.
Se falhar, parar aqui e guardar o log; o BBD não participou dessa tentativa.

## Teste 2: BBD sem calibração

Somente se o teste anterior passou:

```sh
rmmod en75xx_slic_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=bbd stage_wait_ms=3000

dmesg | tail -n 120
```

Esperado: `BBD blocks complete, before calibration` e
`after BBD blocks, before calibration` com a versão original, seguidos de
`DXS BBD blocks test passed; calibration pending`. Não deve haver
`opmode request=6` nessa tentativa. Não habilitar ring/linefeed manualmente.

Se houver reset antes da segunda consulta, o problema já se manifesta com
os blocos do BBD e/ou seu estado posterior, sem o comando de calibração.

## Teste 3: sequência de voz

Se os dois anteriores passaram:

```sh
rmmod en75xx_slic_dxs &&
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" bringup_mode=voice

dmesg | tail -n 140
```

O novo registro `BBD blocks complete, before calibration` indica a versão
consultada imediatamente antes da calibração. Se ali ainda há 2.1.x e só
depois de opmode=6 aparece BOOT_FIN/ROM, a investigação fica concentrada na
transição para calibração e nas operações que ela desencadeia. Isso não
prova, por si só, falha elétrica nem exclui defeito no protocolo CSI.

Usar o mesmo firmware nos três testes para manter a comparação controlada.
Se quiser confirmar uma janela maior, repetir o diagnóstico com
`stage_wait_ms=30000`. Esses testes não medem fisicamente clock, reset ou
alimentação do SLIC.

## Verificação local

```sh
python3 -m unittest discover -s tests -p 'test_dxs*stage*.py' -v
```

Testes de host compilam trechos C reais com backend/relógio simulados:
ordem dos estágios, parada antes da voz, perda de versão, falha de leitura,
BBD com calibração pendente, calibração obrigatória no caminho normal,
um/dois canais e preservação do erro de calibração. Não substituem build
dos módulos com os headers/configuração do OpenWrt nem teste na placa.
