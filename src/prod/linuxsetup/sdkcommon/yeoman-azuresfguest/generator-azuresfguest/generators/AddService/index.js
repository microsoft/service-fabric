'use strict';

var path      = require('path')
, generators    = require('yeoman-generator')
, yosay     = require('yosay');
var GuestGenerator = generators.Base.extend({
    constructor: function () {
        generators.Base.apply(this, arguments);

        this.desc('Generate Service Fabric guest application template - Add Service');
        var chalk = require('chalk');
        if (this.config.get('projName')) {
        console.log(chalk.green("Setting project name to", this.config.get('projName')));
        } else {
        var err = chalk.red("Project name not found in .yo-rc.json. Exiting ...");
        throw err;
        }
    },

    prompting: function () {
        var done = this.async();

        var prompts = [];

        this.prompt(prompts, function (props) {
            this.props = props;
            this.props.projName = this.config.get('projName');
            done();
        }.bind(this));
    },

    writing: function() {
        var isAddNewService = true;        
        this.composeWith('azuresfguest:guestbinary', {
                options: { isAddNewService: isAddNewService }
        });        
    },

    end: function () {
        this.config.save();
    }
});

module.exports = GuestGenerator;

