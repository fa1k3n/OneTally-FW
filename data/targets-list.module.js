'use strict';

angular.module('targetsList', [
  'ui.bootstrap'
]);

angular.module('targetsList').
  component('targetsList', {
    templateUrl: 'targets-list.template.html',
    controller: 'targetsListCtrl'
   }) 
   
angular.module('targetsList').
  controller('targetsListCtrl', 
    function($scope, $http, $filter) {
        $scope.data = []

        // TODO: Fetch this data
        $scope.supportedTypes = [
          { id: 0, name: "Internal"},
          { id: 1, name: "GoStream D8"}
        ]

        $http.get("/targets")
            .then(function(response) {                
                $scope.data = response.data
            });
         
        $scope.checkName = function(data, id) {

        }

        $scope.checkAddress = function(data, id) {

        }

        $scope.showType = function(target) {
          var selected = [];
          if(target.type) {
            selected = $filter('filter')($scope.supportedTypes, {id: target.type});
          }
          return selected.length ? selected[0].name : 'Not set';
        }

        $scope.saveTarget = function(target) {
          $http.put("/targets/" + target.id, target).then(function(response) {
              $http.put("/commit", {}).then(function(response) {
                showSuccessMessage(target.name + " updated successfully")
              })

              $http.get("/targets/" + target.id)
                .then(function(response) {                
                  let index = $scope.data.findIndex((item) => item.id === target.id)  
                  $scope.data[index] = response.data
                });
          });
        }

        $scope.new = function() {
          $http.post("/targets", {}).then(function(response) {
              alert(JSON.stringify(response))
              $http.put("/commit", {}).then(function(resp) {
                showSuccessMessage(response.id + " created successfully")
              })

             // $http.get("/targets/" + target.id)
             //   .then(function(response) {                
             //     let index = $scope.data.findIndex((item) => item.id === target.id)  
             //     $scope.data[index] = response.data
             //   });
            })
        }
          
        $scope.delete_target = function(name) {
          $http.delete("/targets/" + name).then(function(response) {
            showSuccessMessage(name + " deleted successfully")
            $http.get("/targets")
              .then(function(response) {                
                $scope.data = response.data
              });
          }, function(resp) {
            alert ("Failed to delete " + resp)
          })
        }
  })
    