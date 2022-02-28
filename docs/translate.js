language_list = ["en","fr"];

// The below provided options are default.
var translator = new Translator({
  defaultLanguage: "en",
  detectLanguage: true,
  selector: "[data-i18n]",
  debug: true,
  registerGlobally: "__",
  persist: false,
  persistKey: "preferred_language",
  filesLocation: "i18n"
});

translator.fetch(language_list).then(() => {
  // Calling `translatePageTo()` without any parameters
  // will translate to the default language.
  if (language_list.includes((translator.defaultLanguage))){
    translator.translatePageTo();
  } else {
      translator.translatePageTo("en");
  }
});