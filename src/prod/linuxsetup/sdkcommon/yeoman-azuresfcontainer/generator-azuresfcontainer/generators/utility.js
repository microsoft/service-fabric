module.exports = {
    validateFQN: function (input) {
        var chalk = require('chalk');
        if(/^(([a-z]|_[a-z0-9_])[a-z0-9_]*(\.([a-z]|_[a-z0-9_])[a-z0-9_]*)*)$/i.test(input)) {
            return true;
        }
        var err = chalk.red("Invalid FQN! Please enter a valid class FQN");
        throw new Error(err);
    },
    capitalizeFirstLetter: function (input) {
        return input.charAt(0).toUpperCase() + input.slice(1);
    },
    uncapitalizeFirstLetter: function (input) {
        return input.charAt(0).toLowerCase() + input.slice(1);
    }
};
