'use strict';

angular.module('peripheralDetails', ['ui.bootstrap']);
angular.module('peripheralDetails').
    component('peripheralDetails', {
        templateUrl: 'peripheral-details.templ.html',
        bindings: {
            resolve: '<',
            close: '&',
            dismiss: '&'
        },
        controller: function () {
            var $ctrl = this;

            $ctrl.$onInit = function () {
                 alert("onInit " + JSON.stringify($ctrl))
                //$ctrl.data = $ctrl.resolve.data;
               
            //$ctrl.selected = {
            //    item: $ctrl.data[0]
            //};
            };

            $ctrl.ok = function () {
                $ctrl.close();
            };

            $ctrl.cancel = function () {
                $ctrl.dismiss();
            };
        }
    })

angular.module('peripheralDetails').
    controller('peripheralDetailsCtrl', function($scope, $http, $uibModalInstance, name, data) {
        var $ctrl = this
        $scope.data = Object.assign({}, data)
        $scope.name = name
    
        $ctrl.ok = function () {
            //$http.put("/peripherals/" + name, $scope.data).then(function(response) {
            //    showSuccessMessage("Peripheral updated successfully")
            //})
            $uibModalInstance.close($scope.data);
        };

        $ctrl.cancel = function () {
           $uibModalInstance.dismiss('cancel');
        };
    })
