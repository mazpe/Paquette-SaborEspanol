package Paquette::Controller::Admin::Customers;

use Moose;
BEGIN { extends 'Catalyst::Controller' }
use Paquette::Form::Admin::Customer;
use Data::Dumper;

has 'customer_form' => (
    isa => 'Paquette::Form::Admin::Customer',
    is => 'ro',
    lazy => 1,
    default => sub { Paquette::Form::Admin::Customer->new },
);


=head1 NAME

Paquette::Controller::Admin::Customers - Catalyst Controller

=head1 DESCRIPTION

Catalyst Controller.

=head1 METHODS

=cut


=head2 index

=cut

sub auto : Private {
    my ( $self, $c ) =  @_;

    $c->stash->{wrapper_admin} = 1;
}

sub index :Chained('based') :Path :Args(0) {
    my ( $self, $c ) = @_;

    my $customers = [$c->model('PaquetteDB::Customer')->all];

    $c->stash->{customers} = $customers;

    $c->stash->{template} = 'admin/customers_list.tt2';

    return;
}

sub base :Chained('/') :PathPart('admin/customers') :CaptureArgs(0) {
    my ( $self, $c ) = @_;

}

sub load : Chained('base') :PathPart('') :CaptureArgs(1) {
    my ( $self, $c, $id ) = @_;
    my $customer;

    if($id) {

        $c->stash->{customer_id} = $id;
        $customer    = $c->model('PaquetteDB::Customer')->find($id);

    } else {

        $c->response->status(404);
        $c->detach;

    }

    if ($customer) {

        $c->stash->{'customer'} = $customer;

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}

sub create : Local {
    my ( $self, $c ) = @_;
    my $row;
    my $form;

    # Get a new empty row
    $row = $c->model('PaquetteDB::Customer')->new_result({});

    # Set our template and form to use
    $c->stash(
        template    => 'admin/customer.tt2',
        form        => $self->customer_form,
    );

    # Process our form
    $form =  $self->customer_form->process (
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
    my $customer_id;
    my $row;
    my $form;

    $customer_id = $c->stash->{customer_id};
    # Get a new empty row
    $row = $c->model('PaquetteDB::Customer')->find_customer($customer_id);

    # Set our template and form to use
    $c->stash(
        template    => 'admin/customer.tt2',
        form        => $self->customer_form,
    );

    # Process our form
    $form =  $self->customer_form->process (
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
    my $cmd;
    my $customer = $c->stash->{customer};

    if ($customer) {

        my $is_deleted = $customer->delete;
        
        $c->response->redirect(
            $c->uri_for( $self->action_for('index')) . '/' );

    } else {

        $c->response->status(404);
        $c->detach;

    }

    return;
}


=head1 AUTHOR

Lester Ariel Mesa,,305-402-6717,

=head1 LICENSE

This library is free software. You can redistribute it and/or modify
it under the same terms as Perl itself.

=cut

1;
