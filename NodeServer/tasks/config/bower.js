module.exports = function(grunt){
  grunt.config.set('bower', {
    dev: {
      dest: 'assets',
      js_dest: 'assets/js',
      css_dest: 'assets/css'
    }
  });

  grunt.loadNpmTasks('grunt-bower');

};
