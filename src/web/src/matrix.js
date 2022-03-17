class MatrixManager {
  constructor() {
    this.getAddress();
    this.getType();
  }

  setupMatrix() {
    if (this.type == "M5Atom") {
      this.width = 5;
      this.height = 5;
      this.top = true;
      this.left = false;
      this.zigzag = false;
    } else {
      this.width = 16;
      this.height = 16;
      this.top = false;
      this.left = false;
      this.zigzag = true;
    }

    this.nb = this.width * this.height;

    this.selected_color = 8;
    this.selected_color_hex = "white";

    this.last_button = document.getElementById("color_8");
    this.last_button.style.transform = "translateY(4px)";
    this.last_button.style.boxShadow = "0 5px #666";

    this.generateClearCommandArray();
    this.generateMatrixMapping();
    this.commandArrayToCommandString();
    this.generateTableMatrix();
    this.loadPresets();

    this.qrcode = new QRCode(document.getElementById("qrcode"), {
      text: this.address,
      correctLevel: QRCode.CorrectLevel.L
    });

    this.setHTTPrequest();
    this.sendCommand();
    this.qrcode.makeCode(this.address);
    console.log(matrix);
  }

  async getType() {
    const response = await fetch(this.address + "/" + "type");
    this.type = await response.text();
    console.log("Type: " + this.type);
    this.setupMatrix();
  }

  async mode(mode) {
    fetch(this.address + "/" + "mode?" + mode);
  }

  generateMatrixMapping() {
    console.log("Generate leds mapping");
    var zigzagged = false;
    this.mapping = [];

    if (this.top && !this.left && !this.zigzag) {
      var height_mapper = this.height - 1;
      for (var p = 0; p < this.height; p++) {
        this.mapping[p] = this.height * (height_mapper);
        height_mapper--;
      }
      for (var p = this.height; p < this.nb; p++) {
        this.mapping[p] = this.mapping[p - this.height] + 1;
      }
    }

    if (!this.top && !this.left && this.zigzag) {
      var height_mapper = this.height - 1;
      for (var p = 0; p < this.height; p++) {
        if (zigzagged) {
          this.mapping[p] = this.mapping[p - 1] - 1;
          zigzagged = false;
        } else {
          this.mapping[p] = this.height * (height_mapper)
          height_mapper = height_mapper - 2;
          zigzagged = true;
        }
      }
      for (var p = this.height; p < (this.nb); p++) {
        if (zigzagged) {
          this.mapping[p] = this.mapping[p - this.height] - 1;
          zigzagged = false;
        } else {
          this.mapping[p] = this.mapping[p - this.height] + 1;
          zigzagged = true;
        }
      }
    }
  }

  generateClearCommandArray() {
    console.log("Generate empty matrix");
    this.commandArray = [];
    for (var i = 0; i < (this.nb); i++) {
      this.commandArray[i] = 0;
    }
  }

  generateTableMatrix() {
    console.log("Generate table");
    var html_table = "";
    var size = 26;
    if (this.width <= 8) {
      size = 96;
    }
    var pix = 0;
    for (var h = 0; h < this.height; h++) {
      html_table += "<tr>";
      for (var w = 0; w < this.width; w++) {
        html_table += `
            <td id="pixel_${this.mapping[pix]}"
            style="background-color: rgb(0,0,0);width:${size}px;height:${size}px"
            onclick="matrix.draw(this,${this.mapping[pix]},qrcode)"
            >
            </td>
            `;
        pix++;
      }
      html_table += "</tr>";
    }
    document.getElementById("matrix_table").innerHTML = html_table;
  }

  loadPresets() {
    this.presets = [];
    this.presets_name = [];
    if (localStorage.getItem("m5atomkhey_presets") !== null) {
      this.presets = JSON.parse(localStorage.getItem("m5atomkhey_presets"))
      this.presets_name = JSON.parse(localStorage.getItem("m5atomkhey_presets_name"))
      var preset_id = 0;
      document.getElementById("presets").innerHTML = "";
      for (var preset of this.presets) {
        document.getElementById("presets").innerHTML += `<button onclick='matrix.redraw(${preset_id})' class='command'>${this.presets_name[preset_id]}</button>`;
        document.getElementById("presets").innerHTML += `<button onclick='matrix.removePreset(${preset_id})' class='command'>X</button>&nbsp;&nbsp;&nbsp;&nbsp;`;
        preset_id++;
      }
    }
    console.log("Presets ----");
    console.log(this.presets_name, this.presets);
    console.log("------------");
  }

  redraw(preset_id) {
    this.commandArray = this.presets[preset_id];
    for(var i = 0; i < this.commandArray.length; i++) {
        var obj = document.getElementById("pixel_" + i);
        obj.style.backgroundColor = document.getElementById("color_" + this.commandArray[i]).style.backgroundColor;
    }
    this.setHTTPrequest();
    this.sendCommand();
  }

  removePreset(preset_id) {
    this.presets.splice(preset_id, 1);
    this.presets_name.splice(preset_id, 1);
    localStorage.setItem("m5atomkhey_presets", JSON.stringify(this.presets));
    localStorage.setItem("m5atomkhey_presets_name", JSON.stringify(this.presets_name));
    this.loadPresets();
  }

  savePresets() {
    var preset_name = prompt("Preset name", "newpreset");
    console.log("Save preset :" + preset_name);
    var id_preset = -1;
    for (var i = 0; i < this.presets_name.length; i++) {
      if(this.presets_name[i] == preset_name) {
        id_preset = i;
      }
    }
    console.log(id_preset);
    if(id_preset == -1) {
      this.presets_name.push(preset_name);
      this.presets.push(this.commandArray);
      alert("Preset saved");
    } else {
      this.presets[id_preset] = this.commandArray;
      alert("Preset changed");
    }
    localStorage.setItem("m5atomkhey_presets", JSON.stringify(this.presets));
    localStorage.setItem("m5atomkhey_presets_name", JSON.stringify(this.presets_name));
    this.loadPresets();
  }

  commandArrayToCommandString() {
    this.commandString = ""
    for (var i = 0; i < (this.nb); i++) {
      this.commandString += this.commandArray[i]
    }
    this.commandURL = this.address + "/draw?pixels=" + this.commandString
  }

  setHTTPrequest() {
    this.commandArrayToCommandString();
    this.qrcode.clear();
    this.qrcode.makeCode(this.commandURL);
    document.getElementById("value").value = this.commandURL;
  }

  async sendCommand() {
    console.log("Send: " + this.commandURL);
    fetch(this.commandURL).catch(function (error) {
      //console.log("RESEND NEED");
      //console.log(error);
      document.getElementById("error").innerHTML = error;
    });
  }

  draw(obj, nb) {
    console.log("Draw " + nb);
    if (this.commandArray[nb] != this.selected_color) {
      obj.style.backgroundColor = this.selected_color_hex;
      this.commandArray[nb] = this.selected_color;
    } else {
      obj.style.backgroundColor = "#000000";
      this.commandArray[nb] = 0;
    }
    this.setHTTPrequest();
    this.sendCommand();
  }

  getUrlVars() {
    var vars = {};
    var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function (m, key, value) {
      vars[key] = value;
    });
    return vars;
  }

  getUrlParam(parameter) {
    var urlparameter = "";
    if (window.location.href.indexOf(parameter) > -1) {
      urlparameter = this.getUrlVars()[parameter];
    }
    return urlparameter;
  }

  getAddress() {
    this.address = "";
    if (this.getUrlParam("ip") != "") {
      this.address = "http://" + this.getUrlParam("ip");
    } else {
      this.address = window.document.location.origin
    }
    console.log("URL: " + this.address);
  }

  changeColor(newcolor, obj) {
    this.last_button.style.transform = "";
    this.last_button.style.boxShadow = "0 9px #999";

    this.selected_color = newcolor;
    this.selected_color_hex = obj.style.backgroundColor;
    obj.style.transform = "translateY(4px)";
    obj.style.boxShadow = "0 5px #666";
    this.last_button = obj;
  }
}

function goBack() {
  window.location.href = "https://usini.github.io/M5atomKhey?ip=" + window.document.location.host
}