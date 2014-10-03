package Paquette::Controller::Admin::States;

use Moose;
BEGIN { extends 'Catalyst::Controller' }
use Paquette::Form::Admin::State;
use Data::Dumper;

has 'state_form' => (
    isa => 'Paquette::Form::Admin::State',
    is => 'ro',
    lazy => 1,
    default => sub { Paquette::Form::Admin::State->new },
);


=head1 NAME

Paquette::Controller::Admin::States - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut


sub index :Chained('base') :Path :Args(0) {
    my ( $self, $c ) = @_;

    my $states = [$c->model('PaquetteDB::States')->all];

    $c->stash->{'states'}    = $states;
    $c->stash->{wrapper_admin}  = "1";
    $c->stash->{template} = 'admin/states_list.tt2';

    return;
}

sub base :Chained('/') :PathPart('admin/states') :CaptureArgs(0) {
    my ( $self, $c ) = @_;

    $c->stash->{wrapper_admin}  = "1";
}

sub load : Chained('base') :PathPart('') :CaptureArgs(1) {
    my ( $self, $c, $id ) = @_;
    my $state;

    if($id) {

        $c->stash->{state_id} = $id;
        $state    = $c->model('PaquetteDB::States')->find($id);

    } else {

        $c->response->status(404);
        $c->detach;

    }

    if ($state) {

        $c->stash->{'state'} = $state;

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub creation : Local {
    my ( $self, $c ) = @_;
    my $row;
    my $form;

$c->stash->{wrapper_admin}  = "1";

    # Get a new empty row
    $row = $c->model('PaquetteDB::States')->new_result({});

    # Set our template and form to use
    $c->stash(
        template    => 'admin/state.tt2',
        form        => $self->state_form,
    );

    # Process our form
    $form =  $self->state_form->process (
        item            => $row,
        params          => $c->req->params,
    );

    # If the form has been submited sucessfully, then redirect to confirm page
    if ($form) {
        $c->res->redirect( $c->uri_for($self->action_for('index')) );
    }

}

sub edit : Chained('load') : PathPart('edit') : Args(0) {
    my ( $self, $c ) = @_;
    my $state_id;
    my $form;
    my $row;

    $state_id = $c->stash->{state_id};

    # Get a new empty row
    #$row = $c->model('PaquetteDB::States')->get_state($state_id);
    $row = $c->stash->{state};

    # Set our template and form to use
    $c->stash(
        template    => 'admin/state.tt2',
        form        => $self->state_form,
    );

    # Process our form
    $form =  $self->state_form->process (
        item            => $row,
        params          => $c->req->params,
    );

    # If the form has been submited sucessfully, then redirect to confirm page
    if ($form) {
        $c->res->redirect( $c->uri_for($self->action_for('index')) );
    }

}

sub delete : Chained('load') : PathPart('delete') : Args(0) {
    my ( $self, $c ) = @_;

    my $state = $c->stash->{state};

    if ($state) {

        $state->delete;

        $c->response->redirect(
            $c->uri_for( $self->action_for('index')) . '/' );

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}


=head1 AUTHOR

Lester Ariel Mesa,,lesterm@gmail.com,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
