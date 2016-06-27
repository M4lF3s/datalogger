/**
 * SerialController
 *
 * @author      :: Marcel Fraas
 * @description :: Server-side logic for managing serial interfaces
 */

var SerialPort = require("serialport");

module.exports = {
  _config: {
    actions: true,
    shortcuts: false,
    rest: false
  },

  // Lists all available COM-Ports
  list: function (req, res) {
    SerialPort.list(function (err, ports) {
      var obj = [];
      ports.forEach(function(port) {
        // console.log(port.comName);
        // console.log(port.pnpId);
        // console.log(port.manufacturer);
        // console.log("=====================");
        obj.push({
          'comName': port.comName,
          'pnpId': port.pnpId,
          'manufacturer': port.manufacturer
        });
      });
      return res.send(obj);
    });
  },


  // Opens a Connection to the specified Port (params.name) and Baudrate (params.baud)
  // See SerialService for implementation
  open: function (req, res) {
    var params = req.params.all();
    console.log('Connection to: ' + params.name + " with Baudrate: " + params.baud);
    SerialService.open({
      'name': params.name,
      'baudrate': params.baud
    });
    res.send(200);
  }
};

