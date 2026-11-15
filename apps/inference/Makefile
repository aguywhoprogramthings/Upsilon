apps += Inference::App
app_headers += apps/inference/app.h

app_inference_test_src = apps/inference/statistics.cpp

app_inference_src = $(addprefix apps/inference/,\
  app.cpp \
  parameters_controller.cpp \
  results_controller.cpp \
  statistic_controller.cpp \
)

app_inference_src += $(app_inference_test_src)
apps_src += $(app_inference_src)

i18n_files += $(call i18n_with_universal_for,inference/base)

tests_src += apps/inference/test/statistics.cpp

$(eval $(call depends_on_image,apps/inference/app.cpp,apps/inference/inference_icon.png))
