# DXS: separar o tom interno do caminho PCM

Este patch adiciona um teste de isolamento do áudio. Não é uma correção
confirmada do RX/DMA e não deve ser acompanhado por alterações de slots.

## O que muda

- O pacote MaxLinear passa a compilar o gerador de tons explicitamente,
  com `--enable-tone-generator`. O grupo inteiro de recursos estendidos
  continua desabilitado, preservando a configuração de ring já validada.
- A ponte DXS pode expor `EN75XX_VOICE_SET_TONE` por TAPI, usando
  `IFX_TAPI_TONE_PLAY` com módulo ALM e direção externa. Esse ioctl recebe
  um ponteiro para a estrutura, diferente dos comandos TAPI3 escalares.
- `hardware_tones=1` é necessário ao carregar a ponte. O padrão é zero:
  sem o parâmetro, os recursos anunciados e o caminho anterior continuam.
- `en75xx-test DEVICE hwtone SECONDS HZ` gera um tom de -24 dBm no chip.
  Não escreve nem lê amostras PCM e não faz fallback para tom por software.
  Para parada, usa o ioctl e também a limpeza já existente no release.
- Um índice de tabela distinto é reservado por canal/dispositivo. Não use
  outra aplicação TAPI para reprogramar esses índices durante o teste.

O backend MaxLinear `DXS_TAPI_LL_ALM_TG_Play()` usa o gerador DTMF/AT interno
com mute do PCM recebido. `DXS_TAPI_LL_ALM_TG_Stop()` desliga o gerador e
restaura a recepção PCM. O controle CSI e seus clocks continuam necessários;
o teste isola a origem das amostras de áudio, não desliga a interface física.

Firmware, BBD, DC/DC, parâmetros elétricos e PCM/DMA não são modificados.
O tom é audível no fone: este recurso não é o sinal elétrico de ring.

## Aplicação e compilação

Aplicar na raiz do repositório airoha_voip, com o patch de ring engine já
aplicado. O patch de preserve_interface deve continuar aplicado também.
O patch anterior de native SLIN não é necessário para este teste.

```sh
git apply --check /caminho/nokia-dxs-internal-tone.patch
git apply /caminho/nokia-dxs-internal-tone.patch
```

Se o código foi copiado para OpenWrt, copie novamente os arquivos alterados
de src e os dois pacotes. Se usa src-link, mantenha o mesmo fluxo do feed.
Na árvore OpenWrt, para pacotes copiados nos caminhos usuais:

```sh
make package/kernel/en75xx-voip/clean
make package/kernel/en75xx-voip/compile V=s
make package/en75xx-fxs-bench-test/clean
make package/en75xx-fxs-bench-test/compile V=s
```

Para feed, use os caminhos correspondentes em package/feeds/SEU_FEED.
Instale o drv_tapi_dxs.ko, a ponte en75xx-slic-dxs.ko e o en75xx-test
recompilados. Recompilar somente a ponte não habilita o gerador no TAPI.
Os módulos precisam corresponder ao kernel em execução.

## Teste na Nokia EN751221 / PEF32002 já validada

1. Pare Asterisk se estiver em execução e coloque ambos os telefones no
   gancho antes de recarregar os módulos. Não remova módulos ocupados.
2. Mantenha RX_SLOT0 em 0x10301020 e IFACE_CTRL em 0xf5071306.
   Não repita o experimento RX=33: ele não produziu áudio válido.
3. Mantenha preserve_interface=1 e ALC/LEC desligados.
4. Habilite a opção na ponte antes do teste:

```sh
echo 1 > /sys/module/en75xx_pcm/parameters/preserve_interface
echo 0 > /sys/module/en75xx_pcm/parameters/alc_enable
echo 0 > /sys/module/en75xx_pcm/parameters/lec_enable

# Apenas se estiverem carregados; caso falhe, não force a remoção.
rmmod en75xx_slic_dxs
rmmod drv_tapi_dxs
modprobe drv_tapi_dxs
insmod "/lib/modules/$(uname -r)/en75xx-slic-dxs.ko" \
    bringup_mode=voice hardware_tones=1

en75xx-test /dev/en75xx-fxs0 info
```

Deve aparecer caps=0x1f. Se continuar 0xf, a ponte não foi atualizada ou
não recebeu hardware_tones=1. Depois, tire FXS0 do gancho e execute:

```sh
en75xx-test /dev/en75xx-fxs0 hwtone 5 425
en75xx-test /dev/en75xx-fxs0 hwtone 5 1000
```

Teste FXS1 separadamente com o mesmo aparelho. Guarde a saída e dmesg.
O comando deve imprimir `SLIC internal tone` e parar ao término ou Ctrl-C.
SET_TONE falhando é uma falha do teste; não prova um defeito analógico.

Abrir o dispositivo ainda inicia DMA conforme ABI v1. Como hwtone não
alimenta TX nem drena RX, os contadores de underrun/overrun podem subir.
Isso é esperado neste comando, e não mede a qualidade do tom interno.

## Interpretação

- Tom interno limpo e tom equivalente pelo PCM distorcido: a etapa de
  saída analógica consegue reproduzir esse tom; investigar primeiro
  formato, timing e transporte PCM. Isso não valida toda a captura RX.
- Ambos distorcidos: investigar também ganhos/coeficientes, alimentação,
  aparelho e caminho analógico. O resultado sozinho não identifica a causa.
- Tom interno não inicia ou não para: guardar o erro TAPI; não concluir
  nada sobre a qualidade do codec a partir de um teste que falhou.

Para comparar 1 kHz, use os probes BE/LE anteriores com Asterisk parado.
Não compare apenas volumes: o tom interno usa -24 dBm; o arquivo usa uma
amplitude digital e pode atravessar ganhos diferentes.

O Asterisk existente pode usar o tom interno automaticamente para discar
quando a capacidade é anunciada e hardware_tones está habilitado na sua
configuração. Isso pode melhorar o dial tone, mas não corrige a voz PCM.
Para retornar ao comportamento anterior, recarregue a ponte sem
hardware_tones=1, com Asterisk parado e as linhas livres.

## Evidência e validação

Conferido contra os fontes MaxLinear fornecidos: drv_tapi_io.h,
drv_tapi_ioctl_handlers.h, drv_tapi_tone.c, drv_dxs_tg.c e drv_dxs_dtmf.c.
O SDK pcm_ext_conf.h identifica bits 17/16 de IFACE_CTRL como byteOrder e
bitOrder, respectivamente; não devem ser tratados como edges de TX/RX.

Compilação local do bench com -Wall -Wextra -Werror. Harness do handler
compilado contra os tipos TAPI3 reais, validando direção ALM, conversão
de nível, índices distintos, cadência, parâmetros inválidos e propagação
de erros. Harness do bench validando término por tempo, gancho, falha e
interrupção, incluindo limpeza após tentativa de início com erro.

Sem cross-build OpenWrt/MIPS nem validação física nesta etapa. O patch
foi conferido sobre main (ponte 6ceb155e, bench 588dde57) mais ring engine,
e também com os patches locais de calibração e preserve_interface.
