# MaxLinear DXS firmware and coefficients

These files target the PEF32001 (DXS101) and PEF32002 (DXS102).

`DXS_BBD.bin` is not a generic coefficient image. Its SHA-256 is
`e0c84932582aa8a2931c59d910896f9b1184d189f59af1574ef8fc586354261d` and it
matches the MaxLinear `coef_dxs-0.0.9` profile
`dcdc_CIBB12/R600.bin` byte for byte. It may only be selected for a board that
actually uses the CIBB12 converter topology and a 600-ohm line profile.

`DXS_FW.bin` is the 16,984-byte firmware recovered from the Nokia G-240G-E.
Its SHA-256 is
`708f929decc967544c03295dbc7601ec6eda24c7197166d1d757afcabf3dd882`.
It is different from MaxLinear firmware 2.1.9 (25,200 bytes, SHA-256
`d3a80bcd270e6a8b44cceb97ed780493bb25afa766d831de998c7f1f25c104bf`).
Do not replace it solely because the latter has a newer version number: the
driver selects payloads according to the detected DXS silicon/ROM revision.

The MaxLinear binary firmware and coefficient packages have their own license.
Keep locally obtained releases outside the source tree unless their exact
redistribution terms have been reviewed. The driver source under
`vendor/maxlinear/` is separately offered under GPL-2.0 or BSD-2-Clause.
