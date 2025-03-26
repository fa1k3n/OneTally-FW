'use strict';

angular.module('triggerDetails', ['ui.bootstrap']);
angular.module('triggerDetails').
    component('triggerDetails', {
        templateUrl: 'trigger-details.template.html',
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

angular.module('triggerDetails').
    //controller('triggerDetailsCtrl', function($scope, $rootScope, $http, $uibModalInstance, data) {
    controller('triggerDetailsCtrl', function($scope, $http, $uibModalInstance, data, peripherals, events) {
        var $ctrl = this
        $scope.data = Object.assign({}, data)
        $scope.peripherals = Object.assign({}, peripherals)
        $scope.events = Object.assign({}, events)
    
        $ctrl.ok = function () {
            $http.post("/triggers/" + $scope.data["id"], $scope.data).then(function(response) {
                showSuccessMessage("Trigger updated successfully")
            })
            $uibModalInstance.close($scope.data["id"]);
        };

        $ctrl.cancel = function () {
           $uibModalInstance.dismiss('cancel');
        };




      /*  $scope.$on('editTrigger', function(event, data) { 
            alert("SELECTED TRIGGER")
            
            $scope.data = Object.assign({}, data);  // Clone it, then if save overwrite
            //$scope.open()
        });
        
        $scope.saveTrigger = function(id) { 
            $('#editModal').modal('hide')
            $http.post("/triggers/" + $scope.data["id"], $scope.data).then(function(response) {
                showSuccessMessage("Trigger updated successfully")
                //$('#commit_pif_button').prop('disabled', true);
                $rootScope.$broadcast('triggerUpdated', id);
            })
        }*/
    })
/*angular.module('triggerDetails').
  component('triggerDetails', {
    templateUrl: 'trigger-details.template.html',
    controller: ['$scope', '$rootScope', '$http', function triggerDetailsCtrl($scope, $rootScope, $http, $uibModalInstance, items) { 
        $scope.data = {}     

        $scope.$on('editTrigger', function(event, data) { 
            alert("SELECTED TRIGGER")
            
            $scope.data = Object.assign({}, data);  // Clone it, then if save overwrite
            //$scope.open()
        });
        
        $scope.saveTrigger = function(id) { 
            $('#editModal').modal('hide')
            $http.post("/triggers/" + $scope.data["id"], $scope.data).then(function(response) {
                showSuccessMessage("Trigger updated successfully")
                //$('#commit_pif_button').prop('disabled', true);
                $rootScope.$broadcast('triggerUpdated', id);
            })
        }
    }
  ]})*/