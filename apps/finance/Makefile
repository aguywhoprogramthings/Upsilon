apps += Finance::App
app_headers += apps/finance/app.h

app_finance_test_src = apps/finance/tvm.cpp

app_finance_src = $(addprefix apps/finance/,\
  app.cpp \
  parameters_controller.cpp \
  unknown_controller.cpp \
)

app_finance_src += $(app_finance_test_src)
apps_src += $(app_finance_src)

i18n_files += $(call i18n_without_universal_for,finance/base)

tests_src += apps/finance/test/tvm.cpp

$(eval $(call depends_on_image,apps/finance/app.cpp,apps/finance/finance_icon.png))
