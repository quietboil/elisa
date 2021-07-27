COMPONENT_EMBED_FILES := app.html.gz icon.svg.gz

${PROJECT_PATH}/html/app.html.gz: ${PROJECT_PATH}/html/app.html
	gzip --best --keep --force --quiet --no-name $<

${PROJECT_PATH}/html/icon.svg.gz: ${PROJECT_PATH}/html/icon.svg
	gzip --best --keep --force --quiet --no-name $<
