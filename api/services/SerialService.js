var SerialPort = require("serialport").SerialPort;

module.exports = {

  open: function (connection) {
    var port = new SerialPort(connection.name, {
      baudrate: connection.baudrate
    });
    port.on('data', function (data) {
      console.log('Data: ' + data);

      var obj = {};                             // create an empty Object
      var dataString = ''+data;                 // convert the Data to String
      var inputData = dataString.split(',');    // creates an Array by splitting the String
      obj['timestamp'] = inputData[0];          // insert timestamp data
      obj['data'] = inputData[1];               // insert value data


      //Data.create({'data' : ''+data}).exec(function createCB(err, created){
      Data.create(obj).exec(function createCB(err, created){
        console.log('Created Data Entry: ' + created.timestamp + ' ' + created.data);
        Data.publishCreate({id: created.id, createdAt: created.createdAt, data: created.data});
      });
    });
  }
};
