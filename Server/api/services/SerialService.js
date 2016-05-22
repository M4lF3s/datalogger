var SerialPort = require("serialport").SerialPort;

module.exports = {

  open: function (connection) {
    var port = new SerialPort(connection.name, {
      baudrate: connection.baudrate
    });
    port.on('data', function (data) {
      console.log('Data: ' + data);
      Data.create({'data' : ''+data}).exec(function createCB(err, created){
        console.log('Created Data Entry: ' + created.data);
        Data.publishCreate({id: created.id, data: created.data});
      });
    });
  }
};
